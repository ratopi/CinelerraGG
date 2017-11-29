/*
 * CINELERRA
 * Copyright (C) 1997-2015 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include<stdio.h>
#include<stdint.h>
#include<math.h>
#include<string.h>

#include "arraylist.h"
#include "bccmodels.h"
#include "bccolors.h"
#include "clip.h"
#include "edlsession.h"
#include "filexml.h"
#include "crikey.h"
#include "crikeywindow.h"
#include "language.h"
#include "vframe.h"

// chroma interpolated key, crikey

REGISTER_PLUGIN(CriKey)

#if 0
void crikey_pgm(const char *fn,VFrame *vfrm)
{
	FILE *fp = fopen(fn,"w");
	int w = vfrm->get_w(), h = vfrm->get_h();
	fprintf(fp,"P5\n%d %d\n255\n",w,h);
	fwrite(vfrm->get_data(),w,h,fp);
	fclose(fp);
}
#endif

CriKeyPoint::CriKeyPoint(int t, int e, float x, float y)
{
	this->t = t;  this->e = e; this->x = x; this->y = y;
}
CriKeyPoint::~CriKeyPoint()
{
}

CriKeyConfig::CriKeyConfig()
{
	color = 0x000000;
	threshold = 0.5f;
	draw_mode = DRAW_ALPHA;
	drag = 0;
	selected = 0;
}
CriKeyConfig::~CriKeyConfig()
{
	points.remove_all_objects();
}

int CriKeyConfig::equivalent(CriKeyConfig &that)
{
	if( this->color != that.color ) return 0;
	if( !EQUIV(this->threshold, that.threshold) ) return 0;
	if( this->draw_mode != that.draw_mode ) return 0;
	if( this->drag != that.drag ) return 0;
	if( this->points.size() != that.points.size() ) return 0;
	for( int i=0, n=points.size(); i<n; ++i ) {
		CriKeyPoint *ap = this->points[i], *bp = that.points[i];
		if( ap->t != bp->t ) return 0;
		if( ap->e != bp->e ) return 0;
		if( !EQUIV(ap->x, bp->x) ) return 0;
		if( !EQUIV(ap->y, bp->y) ) return 0;
	}
	return 1;
}

void CriKeyConfig::copy_from(CriKeyConfig &that)
{
	this->color = that.color;
	this->threshold = that.threshold;
	this->draw_mode = that.draw_mode;
	this->drag = that.drag;
	this->selected = that.selected;

	points.remove_all_objects();
	for( int i=0,n=that.points.size(); i<n; ++i ) {
		CriKeyPoint *pt = that.points[i];
		add_point(pt->t, pt->e, pt->x, pt->y);
	}
}

void CriKeyConfig::interpolate(CriKeyConfig &prev, CriKeyConfig &next,
		long prev_frame, long next_frame, long current_frame)
{
	this->color = prev.color;
	this->draw_mode = prev.draw_mode;
	this->drag = prev.drag;
	this->selected = prev.selected;

	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->threshold = prev.threshold * prev_scale + next.threshold * next_scale;
 // interpolate rgb components
	float prev_target[3];  set_target(0, prev.color, prev_target);
	float next_target[3];  set_target(0, next.color, next_target);
	float target[3];
	for( int i=0; i<3; ++i )
		target[i] = prev_target[i] * prev_scale + next_target[i] * next_scale;
	set_color(0, target, this->color);

	points.remove_all_objects();
	int prev_sz = prev.points.size(), next_sz = next.points.size();
	for( int i=0; i<prev_sz; ++i ) {
		CriKeyPoint *pt = prev.points[i], *nt = 0;
		float x = pt->x, y = pt->y;
		int k = next_sz;  // associated by tag id in next
		while( --k >= 0 && pt->t != (nt=next.points[k])->t );
		if( k >= 0 ) {
			x = x * prev_scale + nt->x * next_scale;
			y = y * prev_scale + nt->y * next_scale;
		}
		add_point(pt->t, pt->e, x, y);
	}
}

void CriKeyConfig::limits()
{
	bclamp(threshold, 0.0f, 1.0f);
	bclamp(draw_mode, 0, DRAW_MODES-1);
}

int CriKeyConfig::add_point(int t, int e, float x, float y)
{
	int k = points.size();
	if( t < 0 ) {
		t = 0;
		for( int i=k; --i>=0; ) {
			int n = points[i]->t;
			if( n >= t ) t = n + 1;
		}
	}
	points.append(new CriKeyPoint(t, e, x, y));
	return k;
}

void CriKeyConfig::del_point(int i)
{
	points.remove_object_number(i);
}


class FillRegion
{
	class segment { public: int y, lt, rt; };
	ArrayList<segment> stack;

	void push(int y, int lt, int rt) {
		segment &seg = stack.append();
		seg.y = y;  seg.lt = lt;  seg.rt = rt;
	}
	void pop(int &y, int &lt, int &rt) {
		segment &seg = stack.last();
		y = seg.y;  lt = seg.lt;  rt = seg.rt;
		stack.remove();
	}
 
	int w, h;
	uint8_t *data, *mask;
	bool edge_pixel(uint8_t *dp) { return *dp; }

public:
	void fill(int x, int y);
	void run();

	FillRegion(VFrame *d, VFrame *m);
};

FillRegion::FillRegion(VFrame *d, VFrame *m)
{
	w = d->get_w();
	h = d->get_h();
	data = d->get_data();
	mask = m->get_data();
}

void FillRegion::fill(int x, int y)
{
	push(y, x, x);
}

void FillRegion::run()
{
	while( stack.size() > 0 ) {
		int y, ilt, irt;
		pop(y, ilt, irt);
		int ofs = y*w + ilt;
		uint8_t *idp = data + ofs;
		uint8_t *imp = mask + ofs;
		for( int x=ilt; x<=irt; ++x,++imp,++idp ) {
			if( !*imp ) continue;
			*imp = 0;
			if( edge_pixel(idp) ) continue;
			int lt = x, rt = x;
			uint8_t *ldp = idp, *lmp = imp;
			for( int i=lt; --i>=0; ) {
				if( !*--lmp ) break;
				*lmp = 0;  lt = i;
				if( edge_pixel(--ldp) ) break;
			}
			uint8_t *rdp = idp, *rmp = imp;
			for( int i=rt; ++i< w; rt=i,*rmp=0 ) {
				if( !*++rmp ) break;
				*rmp = 0;  rt = i;
				if( edge_pixel(++rdp) ) break;
			}
			if( y+1 <  h ) push(y+1, lt, rt);
			if( y-1 >= 0 ) push(y-1, lt, rt);
		}
	}
}


CriKey::CriKey(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	msk = 0;
	dst = 0;
}

CriKey::~CriKey()
{
	delete engine;
	delete msk;
	delete dst;
}

void CriKeyConfig::set_target(int is_yuv, int color, float *target)
{
	float r = ((color>>16) & 0xff) / 255.0f;
	float g = ((color>> 8) & 0xff) / 255.0f;
	float b = ((color>> 0) & 0xff) / 255.0f;
	if( is_yuv ) {
		float y, u, v;
		YUV::yuv.rgb_to_yuv_f(r,g,b, y,u,v);
		target[0] = y;
		target[1] = u + 0.5f;
		target[2] = v + 0.5f;
	}
	else {
		target[0] = r;
		target[1] = g;
		target[2] = b;
	}
}
void CriKeyConfig::set_color(int is_yuv, float *target, int &color)
{
	float r = target[0];
	float g = target[1];
	float b = target[2];
	if( is_yuv ) {
		float y = r, u = g-0.5f, v = b-0.5f;
		YUV::yuv.yuv_to_rgb_f(y,u,v, r,g,b);
	}
	int ir = r >= 1 ? 0xff : r < 0 ? 0 : (int)(r * 256);
	int ig = g >= 1 ? 0xff : g < 0 ? 0 : (int)(g * 256);
	int ib = b >= 1 ? 0xff : b < 0 ? 0 : (int)(b * 256);
	color = (ir << 16) | (ig << 8) | (ib << 0);
}

void CriKey::get_color(int x, int y)
{
	if( x < 0 || x >= w ) return;
	if( y < 0 || y >= h ) return;
	uint8_t **src_rows = src->get_rows();
	uint8_t *sp = src_rows[y] + x*bpp;
	if( is_float ) {
		float *fp = (float *)sp;
		for( int i=0; i<comp; ++i,++fp )
			target[i] = *fp;
	}
	else {
		float scale = 1./255;
		for( int i=0; i<comp; ++i,++sp )
			target[i] = *sp * scale;
	}
}

const char* CriKey::plugin_title() { return _("CriKey"); }
int CriKey::is_realtime() { return 1; }

NEW_WINDOW_MACRO(CriKey, CriKeyWindow);
LOAD_CONFIGURATION_MACRO(CriKey, CriKeyConfig)

int CriKey::new_point()
{
	EDLSession *session = get_edlsession();
	float x = !session ? 0.f : session->output_w / 2.f;
	float y = !session ? 0.f : session->output_h / 2.f;
	return config.add_point(-1, 0, x, y);
}

void CriKey::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("CRIKEY");
	output.tag.set_property("COLOR", config.color);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("DRAW_MODE", config.draw_mode);
	output.tag.set_property("DRAG", config.drag);
	output.tag.set_property("SELECTED", config.selected);
	output.append_tag();
	output.append_newline();
	output.tag.set_title("/CRIKEY");
	output.append_tag();
	output.append_newline();
	for( int i=0, n = config.points.size(); i<n; ++i ) {
		CriKeyPoint *pt = config.points[i];
		char point[BCSTRLEN];
		sprintf(point,"/POINT_%d",pt->t);
		output.tag.set_title(point+1);
		output.tag.set_property("E", pt->e);
		output.tag.set_property("X", pt->x);
		output.tag.set_property("Y", pt->y);
		output.append_tag();
		output.tag.set_title(point+0);
		output.append_tag();
		output.append_newline();
	}
	output.terminate_string();
}

void CriKey::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->get_data(), strlen(keyframe->get_data()));
	config.points.remove_all_objects();
	int result = 0;

	while( !(result=input.read_tag()) ) {
		if( input.tag.title_is("CRIKEY") ) {
			config.color = input.tag.get_property("COLOR", config.color);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.draw_mode = input.tag.get_property("DRAW_MODE", config.draw_mode);
			config.drag = input.tag.get_property("DRAG", config.drag);
			config.selected = input.tag.get_property("SELECTED", 0);
			config.limits();
		}
		else if( !strncmp(input.tag.get_title(),"POINT_",6) ) {
			int t = atoi(input.tag.get_title() + 6);
			int e = input.tag.get_property("E", 0);
			float x = input.tag.get_property("X", 0.f);
			float y = input.tag.get_property("Y", 0.f);
			config.add_point(t, e, x, y);
		}
	}

	if( !config.points.size() ) new_point();
}

void CriKey::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	thread->window->lock_window("CriKey::update_gui");
	CriKeyWindow *window = (CriKeyWindow*)thread->window;
	window->update_gui();
	window->flush();
	thread->window->unlock_window();
}

void CriKey::draw_alpha(VFrame *msk)
{
	uint8_t **src_rows = src->get_rows();
	uint8_t **msk_rows = msk->get_rows();
	switch( color_model ) {
	case BC_RGB_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				float *px = (float *)sp;
				px[0] = px[1] = px[2] = 0;
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				float *px = (float *)sp;
				px[3] = 0;
			}
		}
		break;
	case BC_RGB888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				uint8_t *px = sp;
				px[0] = px[1] = px[2] = 0;
			}
		}
		break;
	case BC_YUV888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				uint8_t *px = sp;
				px[0] = 0;
				px[1] = px[2] = 0x80;
			}
		}
		break;
	case BC_RGBA8888:
	case BC_YUVA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				if( *mp ) continue;
				uint8_t *px = sp;
				px[3] = 0;
			}
		}
		break;
	}
}

void CriKey::draw_mask(VFrame *msk)
{
	uint8_t **src_rows = src->get_rows();
	uint8_t **msk_rows = msk->get_rows();
	float scale = 1 / 255.0f;
	switch( color_model ) {
	case BC_RGB_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				float *px = (float *)sp;
				px[0] = px[1] = px[2] = a;
			}
		}
		break;
	case BC_RGBA_FLOAT:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				float *px = (float *)sp;
				px[0] = px[1] = px[2] = a;
				px[3] = 1.0f;
			}
		}
		break;
	case BC_RGB888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				uint8_t *px = sp;
				px[0] = px[1] = px[2] = a * 255;
			}
		}
		break;
	case BC_RGBA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				uint8_t *px = sp;
				px[0] = px[1] = px[2] = a * 255;
				px[3] = 0xff;
			}
		}
		break;
	case BC_YUV888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				uint8_t *px = sp;
				px[0] = a * 255;
				px[1] = px[2] = 0x80;
			}
		}
		break;
	case BC_YUVA8888:
		for( int y=0; y<h; ++y ) {
			uint8_t *sp = src_rows[y], *mp = msk_rows[y];
			for( int x=0; x<w; ++x,++mp,sp+=bpp ) {
				float a = *mp * scale;
				uint8_t *px = sp;
				px[0] = a * 255;
				px[1] = px[2] = 0x80;
				px[3] = 0xff;
			}
		}
		break;
	}
}

void CriKey::draw_point(VFrame *src, CriKeyPoint *pt)
{
	int d = bmax(w,h) / 200 + 2;
	int r = d/2+1, x = pt->x, y = pt->y;
	src->draw_smooth(x-r,y+0, x-r, y-r, x+0,y-r);
	src->draw_smooth(x+0,y-r, x+r, y-r, x+r,y+0);
	src->draw_smooth(x+r,y+0, x+r, y+r, x+0,y+r);
	src->draw_smooth(x+0,y+r, x-r, y+r, x-r,y+0);
	if( pt->e ) {
		src->set_pixel_color(RED);
		src->draw_x(pt->x, pt->y, d);
	}
	else {
		src->set_pixel_color(BLUE);
		src->draw_t(pt->x, pt->y, d);
	}
}

int CriKey::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{
	load_configuration();
	src = frame;
	w = src->get_w(), h = src->get_h();
	color_model = src->get_color_model();
	bpp = BC_CModels::calculate_pixelsize(color_model);
	comp = BC_CModels::components(color_model);
        if( comp > 3 ) comp = 3;
	is_yuv = BC_CModels::is_yuv(color_model);
	is_float = BC_CModels::is_float(color_model);

	read_frame(src, 0, start_position, frame_rate, 0);

	set_target(is_yuv, config.color, target);

        if( dst && ( dst->get_w() != w || src->get_h() != h ) ) {
		delete dst;  dst = 0;
        }
        if( !dst )
		dst = new VFrame(w, h, BC_A8, 0);
	dst->clear_frame();

	if( !engine )
		engine = new CriKeyEngine(this,
			PluginClient::get_project_smp() + 1,
			PluginClient::get_project_smp() + 1);
	engine->process_packages();
// copy fill btm/rt edges
	int w1 = w-1, h1 = h-1;
	uint8_t *dp = dst->get_data();
	if( w1 > 0 ) for( int y=0; y<h1; ++y,dp+=w ) dp[w1] = dp[w1-1];
	if( h1 > 0 ) for( int x=0; x<w; ++x ) dp[x] = dp[x-w];
//crikey_pgm("/tmp/dst.pgm",dst);

	if( config.draw_mode != DRAW_EDGE ) {
		if( msk && ( msk->get_w() != w || msk->get_h() != h ) ) {
			delete msk;  msk = 0;
		}
		if( !msk )
			msk = new VFrame(w, h, BC_A8, 0);
		memset(msk->get_data(), 0xff, msk->get_data_size());

		FillRegion fill_region(dst, msk);
		for( int i=0, n=config.points.size(); i<n; ++i ) {
			CriKeyPoint *pt = config.points[i];
			if( !pt->e ) continue;
			float x = pt->x, y = pt->y;
			if( x >= 0 && x < w && y >= 0 && y < h )
				fill_region.fill(x, y);
		}
		fill_region.run();

//crikey_pgm("/tmp/msk.pgm",msk);

		if( config.draw_mode == DRAW_MASK )
			draw_mask(msk);
		else
			draw_alpha(msk);
	}
	else
		draw_mask(dst);

	if( config.drag ) {
		for( int i=0, n=config.points.size(); i<n; ++i ) {
			CriKeyPoint *pt = config.points[i];
			src->set_pixel_color(config.selected == i ? GREEN : WHITE);
			draw_point(src, pt);
		}
	}
	return 0;
}


void CriKeyEngine::init_packages()
{
	int y = 0, h1 = plugin->h-1;
	for(int i = 0; i < get_total_packages(); ) {
		CriKeyPackage *pkg = (CriKeyPackage*)get_package(i++);
		pkg->y1 = y;
		y = h1 * i / LoadServer::get_total_packages();
		pkg->y2 = y;
	}
}

LoadPackage* CriKeyEngine::new_package()
{
	return new CriKeyPackage();
}

LoadClient* CriKeyEngine::new_client()
{
	return new CriKeyUnit(this);
}

#define EDGE_MACRO(type, max, components, is_yuv) \
{ \
	uint8_t **src_rows = src->get_rows(); \
	int comps = MIN(components, 3); \
	float scale = 1.0f/max; \
	for( int y=y1; y<y2; ++y ) { \
		uint8_t *row0 = src_rows[y], *row1 = src_rows[y+1]; \
		uint8_t *outp = dst_rows[y]; \
		for( int v,x=x1; x<x2; *outp++=v,++x,row0+=bpp,row1+=bpp ) { \
			type *r0 = (type*)row0, *r1 = (type*)row1; \
			float a00 = 0, a01 = 0, a10 = 0, a11 = 0; \
			for( int i=0; i<comps; ++i,++r0,++r1 ) { \
				float t = target[i]; \
				a00 += fabs(t - r0[0]*scale); \
				a01 += fabs(t - r0[components]*scale); \
				a10 += fabs(t - r1[0]*scale); \
				a11 += fabs(t - r1[components]*scale); \
			} \
			v = 0; \
			float a = bmin(bmin(a00, a01), bmin(a10, a11)); \
			if( a > threshold ) continue; \
			float b = bmax(bmax(a00, a01), bmax(a10, a11)); \
			if( threshold >= b ) continue; \
			v = 255; \
		} \
	} \
} break


void CriKeyUnit::process_package(LoadPackage *package)
{
	VFrame *src = server->plugin->src;
	int bpp = server->plugin->bpp;
	VFrame *dst = server->plugin->dst;
	uint8_t **dst_rows = dst->get_rows();
	float *target = server->plugin->target;
	float threshold = server->plugin->config.threshold;
	CriKeyPackage *pkg = (CriKeyPackage*)package;
	int x1 = 0, x2 = server->plugin->w-1;
	int y1 = pkg->y1, y2 = pkg->y2;

	switch( src->get_color_model() ) {
	case BC_RGB_FLOAT:  EDGE_MACRO(float, 1, 3, 0);
	case BC_RGBA_FLOAT: EDGE_MACRO(float, 1, 4, 0);
	case BC_RGB888:	    EDGE_MACRO(unsigned char, 0xff, 3, 0);
	case BC_YUV888:	    EDGE_MACRO(unsigned char, 0xff, 3, 1);
	case BC_RGBA8888:   EDGE_MACRO(unsigned char, 0xff, 4, 0);
	case BC_YUVA8888:   EDGE_MACRO(unsigned char, 0xff, 4, 1);
	}
}

