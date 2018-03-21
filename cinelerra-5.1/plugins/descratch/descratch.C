/*
DeScratch - Scratches Removing Filter
Plugin for Avisynth 2.5
Copyright (c)2003-2016 Alexander G. Balakhnin aka Fizick
bag@hotmail.ru  http://avisynth.org.ru

This program is FREE software under GPL licence v2.

This plugin removes vertical scratches from digitized films.
Reworked for cin5 by GG. 03/2018, from the laws of Fizick's
*/


#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "descratch.h"

REGISTER_PLUGIN(DeScratchMain)

DeScratchMain::DeScratchMain(PluginServer *server)
 : PluginVClient(server)
{
	inf = 0;  sz_inf = 0;
	src = 0;  dst = 0;
	tmp_frame = 0;
	blurry = 0;
	overlay_frame = 0;
}

DeScratchMain::~DeScratchMain()
{
	delete [] inf;
	delete src;
	delete dst;
	delete blurry;
	delete tmp_frame;
	delete overlay_frame;
}

const char* DeScratchMain::plugin_title() { return N_("DeScratch"); }
int DeScratchMain::is_realtime() { return 1; }

void DeScratchConfig::reset()
{
	threshold = 24;
	asymmetry = 16;
	min_width = 1;
	max_width = 3;
	min_len = 1;
	max_len = 100;
	max_angle = 45;
	blur_len = 4;
	gap_len = 10;
	mode_y = MODE_ALL;
	mode_u = MODE_NONE;
	mode_v = MODE_NONE;
	mark = 0;
	ffade = 100;
	border = 2;
}

DeScratchConfig::DeScratchConfig()
{
	reset();
}

DeScratchConfig::~DeScratchConfig()
{
}

int DeScratchConfig::equivalent(DeScratchConfig &that)
{
	return threshold == that.threshold &&
		asymmetry == that.asymmetry &&
		min_width == that.min_width &&
		max_width == that.max_width &&
		min_len == that.min_len &&
		max_len == that.max_len &&
		max_angle == that.max_angle &&
		blur_len == that.blur_len &&
		gap_len == that.gap_len &&
		mode_y == that.mode_y &&
		mode_u == that.mode_u &&
		mode_v == that.mode_v &&
		mark == that.mark &&
		ffade == that.ffade &&
		border == that.border;
}
void DeScratchConfig::copy_from(DeScratchConfig &that)
{
	threshold = that.threshold;
	asymmetry = that.asymmetry;
	min_width = that.min_width;
	max_width = that.max_width;
	min_len = that.min_len;
	max_len = that.max_len;
	max_angle = that.max_angle;
	blur_len = that.blur_len;
	gap_len = that.gap_len;
	mode_y = that.mode_y;
	mode_u = that.mode_u;
	mode_v = that.mode_v;
	mark = that.mark;
	ffade = that.ffade;
	border = that.border;
}

void DeScratchConfig::interpolate(DeScratchConfig &prev, DeScratchConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}

LOAD_CONFIGURATION_MACRO(DeScratchMain, DeScratchConfig)

void DeScratchMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
// cause data to be stored directly in text
	output.set_shared_output(keyframe->get_data(), MESSAGESIZE);
// Store data
	output.tag.set_title("DESCRATCH");
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("ASYMMETRY", config.asymmetry);
	output.tag.set_property("MIN_WIDTH", config.min_width);
	output.tag.set_property("MAX_WIDTH", config.max_width);
	output.tag.set_property("MIN_LEN", config.min_len);
	output.tag.set_property("MAX_LEN", config.max_len);
	output.tag.set_property("MAX_ANGLE", config.max_angle);
	output.tag.set_property("BLUR_LEN", config.blur_len);
	output.tag.set_property("GAP_LEN", config.gap_len);
	output.tag.set_property("MODE_Y", config.mode_y);
	output.tag.set_property("MODE_U", config.mode_u);
	output.tag.set_property("MODE_V", config.mode_v);
	output.tag.set_property("MARK", config.mark);
	output.tag.set_property("FFADE", config.ffade);
	output.tag.set_property("BORDER", config.border);
	output.append_tag();
	output.tag.set_title("/DESCRATCH");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void DeScratchMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while( !(result = input.read_tag()) ) {
		if(input.tag.title_is("DESCRATCH")) {
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.asymmetry	 = input.tag.get_property("ASYMMETRY", config.asymmetry);
			config.min_width = input.tag.get_property("MIN_WIDTH", config.min_width);
			config.max_width = input.tag.get_property("MAX_WIDTH", config.max_width);
			config.min_len	 = input.tag.get_property("MIN_LEN", config.min_len);
			config.max_len	 = input.tag.get_property("MAX_LEN", config.max_len);
			config.max_angle = input.tag.get_property("MAX_ANGLE", config.max_angle);
			config.blur_len	 = input.tag.get_property("BLUR_LEN", config.blur_len);
			config.gap_len	 = input.tag.get_property("GAP_LEN", config.gap_len);
			config.mode_y	 = input.tag.get_property("MODE_Y", config.mode_y);
			config.mode_u	 = input.tag.get_property("MODE_U", config.mode_u);
			config.mode_v	 = input.tag.get_property("MODE_V", config.mode_v);
			config.mark	 = input.tag.get_property("MARK", config.mark);
			config.ffade	 = input.tag.get_property("FFADE", config.ffade);
			config.border	 = input.tag.get_property("BORDER", config.border);
		}
	}
}

void DeScratchMain::get_extrems_plane(int comp, int thresh)
{
	uint8_t **rows = blurry->get_rows();
	int d = config.max_width, d1 = d+1, wd = src_w - d1;
	int bpp = 3, dsz = d * bpp;
	int asym = config.asymmetry;
	if( thresh > 0 ) {	// black (low value) scratches
		for( int y=0; y<src_h; ++y ) {
			uint8_t *ip = inf + y*src_w;
			int x = 0;
			for( ; x<d1; ++x ) *ip++ = SD_NULL;
			uint8_t *dp = rows[y] + x*bpp + comp;
			for( ; x<wd; ++x,dp+=bpp ) {
				uint8_t *lp = dp-dsz, *rp = dp+dsz;
				*ip++ = (lp[0]-*dp) > thresh && (rp[0]-*dp) > thresh &&
					(abs(lp[-bpp]-rp[+bpp]) <= asym) &&
					 ((lp[0]-lp[+bpp]) + (rp[0]-rp[-bpp]) >
					  (lp[-bpp]-lp[0]) + (rp[bpp]-rp[0])) ?
						SD_EXTREM : SD_NULL; // sharp extremum found
			}
			for( ; x<src_w; ++x ) *ip++ = SD_NULL;
		}
	}
	else {			// white (high value) scratches
		for( int y=0; y<src_h; ++y ) {
			uint8_t *ip = inf + y*src_w;
			int x = 0;
			for( ; x<d1; ++x ) *ip++ = SD_NULL;
			uint8_t *dp = rows[y] + x*bpp + comp;
			for( ; x<wd; ++x,dp+=bpp ) {
				uint8_t *lp = dp-dsz, *rp = dp+dsz;
				*ip++ = (lp[0]-*dp) < thresh && (rp[0]-*dp) < thresh &&
					(abs(lp[-bpp]-rp[+bpp]) <= asym) &&
					 ((lp[0]-lp[+bpp]) + (rp[0]-rp[-bpp]) <
					  (lp[-bpp]-lp[0]) + (rp[bpp]-rp[0])) ?
						SD_EXTREM : SD_NULL; // sharp extremum found
			}
			for( ; x<src_w; ++x ) *ip++ = SD_NULL;
		}
	}
}

//
void DeScratchMain::remove_min_extrems_plane(int comp, int thresh)
{
	uint8_t **rows = blurry->get_rows();
	int d = config.min_width, d1 = d+1, wd = src_w - d1;
	int bpp = 3, dsz = d * bpp;
	int asym = config.asymmetry;
	if( thresh > 0 ) {	// black (low value) scratches
		for( int y=0; y<src_h; ++y ) {
			uint8_t *ip = inf + y*src_w;
			uint8_t *dp = rows[y] + d1*bpp + comp;
			for( int x=d1; x<wd; ++x,++ip,dp+=bpp ) {
				if( *ip != SD_EXTREM ) continue;
				uint8_t *lp = dp-dsz, *rp = dp+dsz;
				if( (lp[0]-*dp) > thresh && (rp[0]-*dp) > thresh &&
					(abs(lp[-bpp]-rp[+bpp]) <= asym) &&
					 ((lp[0]-lp[+bpp]) + (rp[0]-rp[-bpp]) >
					  (lp[-bpp]-lp[0]) + (rp[bpp]-rp[0])) )
						*ip = SD_NULL; // sharp extremum found
			}
		}
	}
	else {			// white (high value) scratches
		for( int y=0; y<src_h; ++y ) {
			uint8_t *ip = inf + y*src_w;
			uint8_t *dp = rows[y] + d1*bpp + comp;
			for( int x=d1; x<wd; ++x,++ip,dp+=bpp ) {
				if( *ip != SD_EXTREM ) continue;
				uint8_t *lp = dp-dsz, *rp = dp+dsz;
				if( (lp[0]-*dp) < thresh && (rp[0]-*dp) < thresh &&
					(abs(lp[-bpp]-rp[+bpp]) <= asym) &&
					 ((lp[0]-lp[+bpp]) + (rp[0]-rp[-bpp]) <
					  (lp[-bpp]-lp[0]) + (rp[bpp]-rp[0])) )
						*ip = SD_NULL; // sharp extremum found
			}
		}
	}
}

void DeScratchMain::close_gaps()
{
	int len = config.gap_len * src_h / 100;
	for( int y=len; y<src_h; ++y ) {
		uint8_t *ip = inf + y*src_w;
		for( int x=0; x<src_w; ++x,++ip ) {
			if( *ip != SD_EXTREM ) continue;
			uint8_t *bp = ip;			// expand to previous lines in range
			for( int i=len; --i>0; ) *(bp-=src_w) = SD_EXTREM;
		}
	}
}

void DeScratchMain::test_scratches()
{
	int w2 = src_w - 2;
	int min_len = config.min_len * src_h / 100;
	int max_len = config.max_len * src_h / 100;
	int maxwidth = config.max_width*2 + 1;
	int maxangle = config.max_angle;

	for( int y=0; y<src_h; ++y ) {
		for( int x=2; x<w2; ++x ) {
			int ofs = y*src_w + x;			// offset of first candidate
			if( inf[ofs] != SD_EXTREM ) continue;
			int ctr = ofs+1, nctr = ctr;		// centered to inf for maxwidth=3
			int hy = src_h - y, len;
			for( len=0; len<hy; ++len ) {		// cycle along inf
				uint8_t *ip = inf + ctr;
				int n = 0;			// number good points in row
				if( maxwidth >= 3 ) {
					if( ip[-2] == SD_EXTREM ) { ip[-2] = SD_TESTED; nctr = ctr-2; ++n; }
					if( ip[+2] == SD_EXTREM ) { ip[+2] = SD_TESTED; nctr = ctr+2; ++n; }
				}
				if( ip[-1] == SD_EXTREM ) { ip[-1] = SD_TESTED; nctr = ctr-1; ++n; }
				if( ip[+1] == SD_EXTREM ) { ip[+1] = SD_TESTED; nctr = ctr+1; ++n; }
				if( ip[+0] == SD_EXTREM ) { ip[+0] = SD_TESTED; nctr = ctr+0; ++n; }
				// end of points tests, check result for row:
				// check gap and angle, if no points or big angle, it is end of inf
				if( !n || abs(nctr%src_w - x) >= maxwidth+len*maxangle/57 ) break;
				ctr = nctr + src_w;		 // new center for next row test
			}
			int mask = len >= min_len && len <= max_len ? SD_GOOD : SD_REJECT;
			ctr = ofs+1; nctr = ctr;		// pass2
			for( len=0; len<hy; ++len ) {		// cycle along inf
				uint8_t *ip = inf + ctr;
				int n = 0;			// number good points in row
				if( maxwidth >= 3 ) {
					if( ip[-2] == SD_TESTED ) { ip[-2] = mask; nctr = ctr-2; ++n; }
					if( ip[+2] == SD_TESTED ) { ip[+2] = mask; nctr = ctr+2; ++n; }
				}
				if( ip[-1] == SD_TESTED ) { ip[-1] = mask; nctr = ctr-1; ++n; }
				if( ip[+1] == SD_TESTED ) { ip[+1] = mask; nctr = ctr+1; ++n; }
				if( ip[+0] == SD_TESTED ) { ip[+0] = mask; nctr = ctr+0; ++n; }
				// end of points tests, check result for row:
				// check gap and angle, if no points or big angle, it is end of inf
				if( !n || abs(nctr%src_w - x) >= maxwidth+len*maxangle/57 ) break;
				ctr = nctr + src_w;		 // new center for next row test
			}
		}
	}
}

void DeScratchMain::mark_scratches_plane(int comp, int mask, int value)
{
	int bpp = 3, dst_w = dst->get_w(), dst_h = dst->get_h();
	uint8_t **rows = dst->get_rows();
	for( int y=0; y<dst_h; ++y ) {
		uint8_t *dp = rows[y] + comp;
		uint8_t *ip = inf + y*src_w;
		for( int x=0; x<dst_w; ++x,++ip,dp+=bpp ) {
			if( *ip == mask ) *dp = value;
		}
	}
}

void DeScratchMain::remove_scratches_plane(int comp)
{
	int r = config.max_width;
	int fade = (config.ffade * 1024) / 100; // norm 2^10
	int fade1 = 1024 - fade;
	uint8_t *ip = inf;
	uint8_t **src_rows = src->get_rows();
	uint8_t **dst_rows = dst->get_rows();
	uint8_t **blur_rows = blurry->get_rows();
	int bpp = 3, margin = r+config.border+2, wm = src_w-margin;
	float nrm = 1. / 1024.f, nrm2r = nrm / (2*r*bpp);

	for( int y=0; y<src_h; ++y ) {
		int left = 0;
		uint8_t *inp = src_rows[y] + comp;
		uint8_t *out = dst_rows[y] + comp;
		uint8_t *blur = blur_rows[y] + comp;
		for( int x=margin; x<wm; ++x ) {
			uint8_t *dp = ip + x;
			if( (dp[+0]&SD_GOOD) && !(dp[-1]&SD_GOOD) ) left = x;
			if( left!=0 && (dp[+0]&SD_GOOD) && !(dp[+1]&SD_GOOD) ) { // the inf, left/right
				int right = x;
				int ctr = (left + right) / 2;			// the inf center
				int ls = ctr - r, rs = ctr + r;
				int lt = ls - config.border - 1, rt = rs + config.border + 1;
				lt *= bpp;  ls *= bpp;  rs *= bpp;  rt *= bpp;  // component index
			 	for( int i=ls; i<=rs; i+=bpp ) {		// across the inf
					int lv = inp[i] + blur[lt] - blur[i];
					int rv = inp[i] + blur[rt] - blur[i];
					lv = fade*lv + fade1*inp[lt];
					rv = fade*rv + fade1*inp[rt];
					int v = nrm2r*(lv*(rs-i) + rv*(i-ls));
			 		out[i] = CLIP(v,0,255);
			 	}
			 	for( int i=lt; i<ls; i+=bpp ) {			 // at left border
					int lv = inp[i] + blur[lt] - blur[i];
					int v = nrm*(fade*lv + fade1*inp[lt]);
			 		out[i] = CLIP(v,0,255);
			 	}
			 	for( int i=rt; i>rs; i-=bpp ) {			// at right border
					int rv = inp[i] + blur[rt] - blur[i];
					int v = nrm*(fade*rv + fade1*inp[rt]);
			 		out[i] = CLIP(v,0,255);
			 	}
				left = 0;
			}
		}
		ip += src_w;
	}
}

void DeScratchMain::pass(int comp, int thresh)
{
// pass for current plane and current sign
	get_extrems_plane(comp, thresh);
	if( config.min_width > 1 )
		remove_min_extrems_plane(comp, thresh);
	close_gaps();
	test_scratches();
	if( config.mark ) {
		int value = config.threshold > 0 ? 0 : 255;
		mark_scratches_plane(comp, SD_GOOD, value);
		mark_scratches_plane(comp, SD_REJECT, 127);
	}
	else
		remove_scratches_plane(comp);
}

void DeScratchMain::blur(int scale)
{
	int tw = src_w, th = (src_h / scale) & ~1;
	if( tmp_frame &&
	    (tmp_frame->get_w() != tw || tmp_frame->get_h() != th) ) {
		delete tmp_frame;  tmp_frame = 0;
	}
	if( !tmp_frame )
		tmp_frame = new VFrame(tw, th, BC_YUV888);

	if( blurry &&
	    (blurry->get_w() != src_w || blurry->get_h() != src_h) ) {
		delete blurry;  blurry = 0;
	}
	if( !blurry )
		blurry = new VFrame(src_w, src_h, BC_YUV888);

	overlay_frame->overlay(tmp_frame, src,
		0,0,src_w,src_h, 0,0,tw,th, 1.f, TRANSFER_NORMAL, LINEAR_LINEAR);
	overlay_frame->overlay(blurry, tmp_frame,
		0,0,tw,th, 0,0,src_w,src_h, 1.f, TRANSFER_NORMAL, CUBIC_CUBIC);
}

void DeScratchMain::copy(int comp)
{
	uint8_t **src_rows = src->get_rows();
	uint8_t **dst_rows = dst->get_rows();
	for( int y=0; y<src_h; ++y ) {
		uint8_t *sp = src_rows[y] + comp, *dp = dst_rows[y] + comp;
		for( int x=0; x<src_w; ++x,sp+=3,dp+=3 ) *sp = *dp;
	}
}

void DeScratchMain::plane_pass(int comp, int mode)
{
	int threshold = config.threshold;
	if( comp != 0 ) threshold /= 2;  // fakey UV scaling
	switch( mode ) {
	case MODE_ALL:
		pass(comp, threshold);
		copy(comp);
	case MODE_HIGH:		// fall thru
		threshold = -threshold;
	case MODE_LOW:		// fall thru
		pass(comp, threshold);
		break;
	}
}

int DeScratchMain::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
	src_w = input->get_w();
	src_h = input->get_h();
	if( src_w >= 2*config.max_width+3 ) {
		if( !overlay_frame ) {
			int cpus = PluginClient::smp + 1;
			if( cpus > 8 ) cpus = 8;
			overlay_frame = new OverlayFrame(cpus);
		}
		if( src && (src->get_w() != src_w || src->get_h() != src_h) ) {
			delete src;  src = 0;
		}
		if( !src ) src = new VFrame(src_w, src_h, BC_YUV888);
		src->transfer_from(input);
		if( dst && (dst->get_w() != src_w || dst->get_h() != src_h) ) {
			delete dst;  dst = 0;
		}
		if( !dst ) dst = new VFrame(src_w, src_h, BC_YUV888);
		dst->copy_from(src);
		int sz = src_w * src_h;
		if( sz_inf != sz ) {  delete [] inf;  inf = 0; }
		if( !inf ) inf = new uint8_t[sz_inf=sz];
		blur(config.blur_len + 1);
		plane_pass(0, config.mode_y);
		plane_pass(1, config.mode_u);
		plane_pass(2, config.mode_v);
		output->transfer_from(dst);
	}
	return 0;
}

void DeScratchMain::update_gui()
{
	if( !thread ) return;
	DeScratchWindow *window = (DeScratchWindow *)thread->get_window();
	window->lock_window("DeScratchMain::update_gui");
	if( load_configuration() )
		window->update_gui();
	window->unlock_window();
}

NEW_WINDOW_MACRO(DeScratchMain, DeScratchWindow)


DeScratchWindow::DeScratchWindow(DeScratchMain *plugin)
 : PluginClientWindow(plugin, 512, 256, 512, 256, 0)
{
	this->plugin = plugin;
}

DeScratchWindow::~DeScratchWindow()
{
}

void DeScratchWindow::create_objects()
{
	int x = 10, y = 10;
	plugin->load_configuration();
	DeScratchConfig &config = plugin->config;

	BC_Title *title;
	add_tool(title = new BC_Title(x, y, _("DeScratch:")));
	y += title->get_h() + 5;
	int x1 = x, x2 = get_w()/2;
	add_tool(title = new BC_Title(x1=x, y, _("threshold:")));
	x1 += title->get_w()+16;
	add_tool(threshold = new DeScratchISlider(this, x1, y, x2-x1-10, 0,64, &config.threshold));
	add_tool(title = new BC_Title(x1=x2, y, _("asymmetry:")));
	x1 += title->get_w()+16;
	add_tool(asymmetry = new DeScratchISlider(this, x1, y, get_w()-x1-15, 0,64, &config.asymmetry));
	y += threshold->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("Mode:")));
	x1 += title->get_w()+16;
	add_tool(title = new BC_Title(x1, y, _("y:")));
	int w1 = title->get_w()+16;
	add_tool(y_mode = new DeScratchMode(this, (x1+=w1), y, &config.mode_y));
	y_mode->create_objects();  x1 += y_mode->get_w()+16;
	add_tool(title = new BC_Title(x1, y, _("u:")));
	add_tool(u_mode = new DeScratchMode(this, (x1+=w1), y, &config.mode_u));
	u_mode->create_objects();  x1 += u_mode->get_w()+16;
	add_tool(title = new BC_Title(x1, y, _("v:")));
	add_tool(v_mode = new DeScratchMode(this, (x1+=w1), y, &config.mode_v));
	v_mode->create_objects();
	y += y_mode->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("width:")));
	w1 = title->get_w()+16;  x1 += w1;
	add_tool(title = new BC_Title(x1, y, _("min:")));
	x1 += title->get_w()+16;
	add_tool(min_width = new DeScratchISlider(this, x1, y, x2-x1-10, 0,16, &config.min_width));
	add_tool(title = new BC_Title(x1=x2, y, _("max:")));
	x1 += title->get_w()+16;
	add_tool(max_width = new DeScratchISlider(this, x1, y, get_w()-x1-15, 0,16, &config.max_width));
	y += min_width->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("len:")));
	w1 = title->get_w()+16;  x1 += w1;
	add_tool(title = new BC_Title(x1, y, _("min:")));
	x1 += title->get_w()+16;
	add_tool(min_len = new DeScratchFSlider(this, x1, y, x2-x1-10, 0.0,100.0, &config.min_len));
	add_tool(title = new BC_Title(x1=x2, y, _("max:")));
	x1 += title->get_w()+16;
	add_tool(max_len = new DeScratchFSlider(this, x1, y, get_w()-x1-15, 0.0,100.0, &config.max_len));
	y += min_len->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("len:")));
	w1 = title->get_w()+16;  x1 += w1;
	add_tool(title = new BC_Title(x1, y, _("blur:")));
	x1 += title->get_w()+16;
	add_tool(blur_len = new DeScratchISlider(this, x1, y, x2-x1-10, 0,16, &config.blur_len));
	add_tool(title = new BC_Title(x1=x2, y, _("gap:")));
	x1 += title->get_w()+16;
	add_tool(gap_len = new DeScratchFSlider(this, x1, y, get_w()-x1-15, 0.0,100.0, &config.gap_len));
	y += blur_len->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("max angle:")));
	w1 = title->get_w()+16;  x1 += w1;
	add_tool(max_angle = new DeScratchFSlider(this, x1, y, x2-x1-10, 0.0,90.0, &config.max_angle));
	add_tool(title = new BC_Title(x1=x2, y, _("fade:")));
	x1 += title->get_w()+16;
	add_tool(ffade = new DeScratchFSlider(this, x1, y, get_w()-x1-15, 0.0,100.0, &config.ffade));
	y += max_angle->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("border:")));
	x1 += title->get_w()+16;
	add_tool(border = new DeScratchISlider(this, x1, y, x2-x1-10, 0,16, &config.border));
	add_tool(mark = new DeScratchMark(this, x1=x2, y));
	w1 = DeScratchReset::calculate_w(this, _("Reset"));
	add_tool(reset = new DeScratchReset(this, get_w()-w1-15, y));

	show_window();
}

void DeScratchWindow::update_gui()
{
	DeScratchConfig &config = plugin->config;
	y_mode->update(config.mode_y);
	u_mode->update(config.mode_u);
	v_mode->update(config.mode_v);
	min_width->update(config.min_width);
	max_width->update(config.max_width);
	min_len->update(config.min_len);
	max_len->update(config.max_len);
	blur_len->update(config.blur_len);
	gap_len->update(config.gap_len);
	max_angle->update(config.max_angle);
	ffade->update(config.ffade);
	mark->update(config.mark);
}


DeScratchModeItem::DeScratchModeItem(DeScratchMode *popup, int type, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->type = type;
}

DeScratchModeItem::~DeScratchModeItem()
{
}

int DeScratchModeItem::handle_event()
{
	popup->update(type);
	return popup->handle_event();
}

DeScratchMode::DeScratchMode(DeScratchWindow *win, int x, int y, int *value)
 : BC_PopupMenu(x, y, 64, "", 1)
{
	this->win = win;
	this->value = value;
}

DeScratchMode::~DeScratchMode()
{
}

void DeScratchMode::create_objects()
{
	add_item(new DeScratchModeItem(this, MODE_NONE, _("None")));
	add_item(new DeScratchModeItem(this, MODE_LOW,  _("Low")));
	add_item(new DeScratchModeItem(this, MODE_HIGH, _("High")));
	add_item(new DeScratchModeItem(this, MODE_ALL,  _("All")));
	set_value(*value);
}

int DeScratchMode::handle_event()
{
	win->plugin->send_configure_change();
	return 1;
}

void DeScratchMode::update(int v)
{
	set_value(*value = v);
}

void DeScratchMode::set_value(int v)
{
	int i = total_items();
	while( --i >= 0 && ((DeScratchModeItem*)get_item(i))->type != v );
	if( i >= 0 ) set_text(get_item(i)->get_text());
}

DeScratchISlider::DeScratchISlider(DeScratchWindow *win,
		int x, int y, int w, int min, int max, int *output)
 : BC_ISlider(x, y, 0, w, w, min, max, *output)
{
        this->win = win;
        this->output = output;
}

DeScratchISlider::~DeScratchISlider()
{
}

int DeScratchISlider::handle_event()
{
        *output = get_value();
        win->plugin->send_configure_change();
        return 1;
}

DeScratchFSlider::DeScratchFSlider(DeScratchWindow *win,
		int x, int y, int w, float min, float max, float *output)
 : BC_FSlider(x, y, 0, w, w, min, max, *output)
{
        this->win = win;
        this->output = output;
}

DeScratchFSlider::~DeScratchFSlider()
{
}

int DeScratchFSlider::handle_event()
{
	*output = get_value();
	win->plugin->send_configure_change();
        return 1;
}

DeScratchMark::DeScratchMark(DeScratchWindow *win, int x, int y)
 : BC_CheckBox(x, y, &win->plugin->config.mark, _("Mark"))
{
	this->win = win;
};

DeScratchMark::~DeScratchMark()
{
}

int DeScratchMark::handle_event()
{
	int ret = BC_CheckBox::handle_event();
	win->plugin->send_configure_change();
	return ret;
}

DeScratchReset::DeScratchReset(DeScratchWindow *win, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->win = win;
}

int DeScratchReset::handle_event()
{
	win->plugin->config.reset();
	win->update_gui();
	win->plugin->send_configure_change();
	return 1;
}

