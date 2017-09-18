
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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
 */

#include "affine.h"
#include "cicolors.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "findobject.h"
#include "findobjectwindow.h"
#include "mutex.h"
#include "overlayframe.h"

#include <errno.h>
#include <exception>
#include <unistd.h>

REGISTER_PLUGIN(FindObjectMain)

FindObjectConfig::FindObjectConfig()
{
	algorithm = NO_ALGORITHM;
	use_flann = 1;
	draw_keypoints = 0;
	draw_border = 0;
	replace_object = 0;
	draw_object_border = 0;
	object_x = 50;  object_y = 50;
	object_w = 100; object_h = 100;
	scene_x = 50;   scene_y = 50;
	scene_w = 100;  scene_h = 100;
	scene_layer = 0;
	object_layer = 1;
	replace_layer = 2;
	blend = 100;
}

void FindObjectConfig::boundaries()
{
	bclamp(object_x, 0, 100);  bclamp(object_y, 0, 100);
	bclamp(object_w, 0, 100);  bclamp(object_h, 0, 100);
	bclamp(scene_x, 0, 100);   bclamp(scene_y, 0, 100);
	bclamp(scene_w, 0, 100);   bclamp(scene_h, 0, 100);
	bclamp(object_layer, MIN_LAYER, MAX_LAYER);
	bclamp(replace_layer, MIN_LAYER, MAX_LAYER);
	bclamp(scene_layer, MIN_LAYER, MAX_LAYER);
	bclamp(blend, MIN_BLEND, MAX_BLEND);
}

int FindObjectConfig::equivalent(FindObjectConfig &that)
{
	int result =
		algorithm == that.algorithm &&
		use_flann == that.use_flann &&
		draw_keypoints == that.draw_keypoints &&
		draw_border == that.draw_border &&
		replace_object == that.replace_object &&
		draw_object_border == that.draw_object_border &&
		object_x == that.object_x && object_y == that.object_y &&
		object_w == that.object_w && object_h == that.object_h &&
		scene_x == that.scene_x && scene_y == that.scene_y &&
		scene_w == that.scene_w && scene_h == that.scene_h &&
		object_layer == that.object_layer &&
		replace_layer == that.replace_layer &&
		scene_layer == that.scene_layer &&
		blend == that.blend;
        return result;
}

void FindObjectConfig::copy_from(FindObjectConfig &that)
{
	algorithm = that.algorithm;
	use_flann = that.use_flann;
	draw_keypoints = that.draw_keypoints;
	draw_border = that.draw_border;
	replace_object = that.replace_object;
	draw_object_border = that.draw_object_border;
	object_x = that.object_x;  object_y = that.object_y;
	object_w = that.object_w;  object_h = that.object_h;
	scene_x = that.scene_x;    scene_y = that.scene_y;
	scene_w = that.scene_w;    scene_h = that.scene_h;
	object_layer = that.object_layer;
	replace_layer = that.replace_layer;
	scene_layer = that.scene_layer;
	blend = that.blend;
}

void FindObjectConfig::interpolate(FindObjectConfig &prev, FindObjectConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}


FindObjectMain::FindObjectMain(PluginServer *server)
 : PluginVClient(server)
{
        affine = 0;
        overlayer = 0;

	cvmodel = BC_RGB888;
	w = h = 0;
        object = scene = replace = 0;
	object_x = object_y = 0;
	object_w = object_h = 0;
	scene_x = scene_y = 0;
	scene_w = scene_h = 0;
	object_layer = 0;
	scene_layer = 1;
	replace_layer = 2;

	border_x1 = 0;  border_y1 = 0;
	border_x2 = 0;  border_y2 = 0;
	border_x3 = 0;  border_y3 = 0;
	border_x4 = 0;  border_y4 = 0;

	obj_x1 = 0;     obj_y1 = 0;
	obj_x2 = 0;     obj_y2 = 0;
	obj_x3 = 0;     obj_y3 = 0;
	obj_x4 = 0;     obj_y4 = 0;

        init_border = 1;
}

FindObjectMain::~FindObjectMain()
{
	delete affine;
	delete overlayer;
}

const char* FindObjectMain::plugin_title() { return _("Find Object"); }
int FindObjectMain::is_realtime() { return 1; }
int FindObjectMain::is_multichannel() { return 1; }

NEW_WINDOW_MACRO(FindObjectMain, FindObjectWindow)
LOAD_CONFIGURATION_MACRO(FindObjectMain, FindObjectConfig)

void FindObjectMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	FindObjectWindow *window = (FindObjectWindow*)thread->window;
	window->lock_window("FindObjectMain::update_gui");
	window->algorithm->set_text(FindObjectAlgorithm::to_text(config.algorithm));
	window->use_flann->update(config.use_flann);
	window->object_x->update(config.object_x);
	window->object_x_text->update((float)config.object_x);
	window->object_y->update(config.object_y);
	window->object_y_text->update((float)config.object_y);
	window->object_w->update(config.object_w);
	window->object_w_text->update((float)config.object_w);
	window->object_h->update(config.object_h);
	window->object_h_text->update((float)config.object_h);
	window->scene_x->update(config.scene_x);
	window->scene_x_text->update((float)config.scene_x);
	window->scene_y->update(config.scene_y);
	window->scene_y_text->update((float)config.scene_y);
	window->scene_w->update(config.scene_w);
	window->scene_w_text->update((float)config.scene_w);
	window->scene_h->update(config.scene_h);
	window->scene_h_text->update((float)config.scene_h);
	window->draw_keypoints->update(config.draw_keypoints);
	window->draw_border->update(config.draw_border);
	window->replace_object->update(config.replace_object);
	window->draw_object_border->update(config.draw_object_border);
	window->object_layer->update( (int64_t)config.object_layer);
	window->replace_layer->update( (int64_t)config.replace_layer);
	window->scene_layer->update( (int64_t)config.scene_layer);
	window->blend->update( (int64_t)config.blend);
	window->flush();
	window->unlock_window();
}

void FindObjectMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("FINDOBJECT");
	output.tag.set_property("ALGORITHM", config.algorithm);
	output.tag.set_property("USE_FLANN", config.use_flann);
	output.tag.set_property("OBJECT_X", config.object_x);
	output.tag.set_property("OBJECT_Y", config.object_y);
	output.tag.set_property("OBJECT_W", config.object_w);
	output.tag.set_property("OBJECT_H", config.object_h);
	output.tag.set_property("SCENE_X", config.scene_x);
	output.tag.set_property("SCENE_Y", config.scene_y);
	output.tag.set_property("SCENE_W", config.scene_w);
	output.tag.set_property("SCENE_H", config.scene_h);
	output.tag.set_property("DRAW_KEYPOINTS", config.draw_keypoints);
	output.tag.set_property("DRAW_BORDER", config.draw_border);
	output.tag.set_property("REPLACE_OBJECT", config.replace_object);
	output.tag.set_property("DRAW_OBJECT_BORDER", config.draw_object_border);
	output.tag.set_property("OBJECT_LAYER", config.object_layer);
	output.tag.set_property("REPLACE_LAYER", config.replace_layer);
	output.tag.set_property("SCENE_LAYER", config.scene_layer);
	output.tag.set_property("BLEND", config.blend);
	output.append_tag();
	output.tag.set_title("/FINDOBJECT");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void FindObjectMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("FINDOBJECT") ) {
			config.algorithm = input.tag.get_property("ALGORITHM", config.algorithm);
			config.use_flann = input.tag.get_property("USE_FLANN", config.use_flann);
			config.object_x = input.tag.get_property("OBJECT_X", config.object_x);
			config.object_y = input.tag.get_property("OBJECT_Y", config.object_y);
			config.object_w = input.tag.get_property("OBJECT_W", config.object_w);
			config.object_h = input.tag.get_property("OBJECT_H", config.object_h);
			config.scene_x = input.tag.get_property("SCENE_X", config.scene_x);
			config.scene_y = input.tag.get_property("SCENE_Y", config.scene_y);
			config.scene_w = input.tag.get_property("SCENE_W", config.scene_w);
			config.scene_h = input.tag.get_property("SCENE_H", config.scene_h);
			config.draw_keypoints = input.tag.get_property("DRAW_KEYPOINTS", config.draw_keypoints);
			config.draw_border = input.tag.get_property("DRAW_BORDER", config.draw_border);
			config.replace_object = input.tag.get_property("REPLACE_OBJECT", config.replace_object);
			config.draw_object_border = input.tag.get_property("DRAW_OBJECT_BORDER", config.draw_object_border);
			config.object_layer = input.tag.get_property("OBJECT_LAYER", config.object_layer);
			config.replace_layer = input.tag.get_property("REPLACE_LAYER", config.replace_layer);
			config.scene_layer = input.tag.get_property("SCENE_LAYER", config.scene_layer);
			config.blend = input.tag.get_property("BLEND", config.blend);
		}
	}

	config.boundaries();
}

void FindObjectMain::draw_line(VFrame *vframe, int x1, int y1, int x2, int y2)
{
	vframe->draw_line(x1, y1, x2, y2);
}

void FindObjectMain::draw_rect(VFrame *vframe, int x1, int y1, int x2, int y2)
{
	draw_line(vframe, x1, y1, x2, y1);
	draw_line(vframe, x2, y1 + 1, x2, y2);
	draw_line(vframe, x2 - 1, y2, x1, y2);
	draw_line(vframe, x1, y2 - 1, x1, y1 + 1);
}

void FindObjectMain::draw_circle(VFrame *vframe, int x, int y, int r)
{
	int x1 = x-r, x2 = x+r;
	int y1 = y-r, y2 = y+r;
	vframe->draw_smooth(x1,y, x1,y1, x,y1);
	vframe->draw_smooth(x,y1, x2,y1, x2,y);
	vframe->draw_smooth(x2,y, x2,y2, x,y2);
	vframe->draw_smooth(x,y2, x1,y2, x1,y);
}

void FindObjectMain::filter_matches(ptV &p1, ptV &p2, double ratio)
{
	DMatches::iterator it;
	for( it=pairs.begin(); it!=pairs.end(); ++it ) { 
		DMatchV &m = *it;
		if( m.size() == 2 && m[0].distance < m[1].distance*ratio ) {
			p1.push_back(obj_keypts[m[0].queryIdx].pt);
			p2.push_back(scn_keypts[m[0].trainIdx].pt);
		}
	}
}

void FindObjectMain::to_mat(Mat &mat, int mcols, int mrows,
	VFrame *inp, int ix,int iy, BC_CModel mcolor_model)
{
	int mcomp = BC_CModels::components(mcolor_model);
	int mbpp = BC_CModels::calculate_pixelsize(mcolor_model);
	int psz = mbpp / mcomp;
	int mdepth = psz < 2 ? CV_8U : CV_16U;
	if( mat.dims != 2 || mat.depth() != mdepth || mat.channels() != mcomp ||
	    mat.cols != mcols || mat.rows != mrows ) {
		mat.release();
	}
	if( mat.empty() ) {
		int type = CV_MAKETYPE(mdepth, mcomp);
		mat.create(mrows, mcols, type);
	}
	uint8_t *mat_rows[mrows];
	for( int y=0; y<mrows; ++y ) mat_rows[y] = mat.ptr(y);
	uint8_t **inp_rows = inp->get_rows();
	int ibpl = inp->get_bytes_per_line(), obpl = mcols * mbpp;
	int icolor_model = inp->get_color_model();
	BC_CModels::transfer(mat_rows, mcolor_model, 0,0, mcols,mrows, obpl,
		inp_rows, icolor_model, ix,iy, mcols,mrows, ibpl, 0);
//	VFrame vfrm(mat_rows[0], -1, mcols,mrows, mcolor_model, mat_rows[1]-mat_rows[0]);
//	static int vfrm_no = 0; char vfn[64]; sprintf(vfn,"/tmp/dat/%06d.png", vfrm_no++);
//	vfrm.write_png(vfn);
}

void FindObjectMain::detect(Mat &mat, KeyPointV &keypts,Mat &descrs)
{
	keypts.clear();
	descrs.release();
	try {
		detector->detectAndCompute(mat, noArray(), keypts, descrs);
	} catch(std::exception e) { printf(_("detector exception: %s\n"), e.what()); }
}

void FindObjectMain::match()
{
	pairs.clear();
	try {
		matcher->knnMatch(obj_descrs, scn_descrs, pairs, 2);
	} catch(std::exception e) { printf(_("match execption: %s\n"), e.what()); }
}

Ptr<DescriptorMatcher> FindObjectMain::flann_kdtree_matcher()
{ // trees=5
	const Ptr<flann::IndexParams>& indexParams =
		makePtr<flann::KDTreeIndexParams>(5);
	const Ptr<flann::SearchParams>& searchParams =
		makePtr<flann::SearchParams>();
	return makePtr<FlannBasedMatcher>(indexParams, searchParams);
}
Ptr<DescriptorMatcher> FindObjectMain::flann_lshidx_matcher()
{ // table_number = 6#12, key_size = 12#20, multi_probe_level = 1#2
	const Ptr<flann::IndexParams>& indexParams =
		makePtr<flann::LshIndexParams>(6, 12, 1);
	const Ptr<flann::SearchParams>& searchParams =
		makePtr<flann::SearchParams>();
	return makePtr<FlannBasedMatcher>(indexParams, searchParams);
}
Ptr<DescriptorMatcher> FindObjectMain::bf_matcher_norm_l2()
{
	return BFMatcher::create(NORM_L2);
}
Ptr<DescriptorMatcher> FindObjectMain::bf_matcher_norm_hamming()
{
	return BFMatcher::create(NORM_HAMMING);
}

#ifdef _SIFT
void FindObjectMain::set_sift()
{
	cvmodel = BC_GREY8;
	detector = SIFT::create();
	matcher = config.use_flann ?
		flann_kdtree_matcher() : bf_matcher_norm_l2();
}
#endif
#ifdef _SURF
void FindObjectMain::set_surf()
{
	cvmodel = BC_GREY8;
	detector = SURF::create(800);
	matcher = config.use_flann ?
		flann_kdtree_matcher() : bf_matcher_norm_l2();
}
#endif
#ifdef _ORB
void FindObjectMain::set_orb()
{
	cvmodel = BC_GREY8;
	detector = ORB::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif
#ifdef _AKAZE
void FindObjectMain::set_akaze()
{
	cvmodel = BC_GREY8;
	detector = AKAZE::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif
#ifdef _BRISK
void FindObjectMain::set_brisk()
{
	cvmodel = BC_GREY8;
	detector = BRISK::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif

void FindObjectMain::process_match()
{
	if( config.algorithm == NO_ALGORITHM ) return;
	if( !config.replace_object &&
	    !config.draw_border &&
	    !config.draw_keypoints ) return;

	if( detector.empty() ) {
		switch( config.algorithm ) {
#ifdef _SIFT
		case ALGORITHM_SIFT:   set_sift();   break;
#endif
#ifdef _SURF
		case ALGORITHM_SURF:   set_surf();   break;
#endif
#ifdef _ORB
		case ALGORITHM_ORB:    set_orb();    break;
#endif
#ifdef _AKAZE
		case ALGORITHM_AKAZE:  set_akaze();  break;
#endif
#ifdef _BRISK
		case ALGORITHM_BRISK:  set_brisk();  break;
#endif
		}
		obj_keypts.clear();  obj_descrs.release();
		to_mat(object_mat, object_w,object_h, object, object_x,object_y, cvmodel);
		detect(object_mat, obj_keypts, obj_descrs);
//printf("detect obj %d features\n", (int)obj_keypts.size());
	}

	to_mat(scene_mat, scene_w,scene_h, scene, scene_x,scene_y, cvmodel);
	detect(scene_mat, scn_keypts, scn_descrs);
//printf("detect scn %d features\n", (int)scn_keypts.size());
	match();
	ptV p1, p2;
	filter_matches(p1, p2);
	if( p1.size() < 4 ) return;
	Mat H = findHomography(p1, p2, RANSAC, 5.0);
	if( !H.dims || !H.rows || !H.cols ) {
//printf("fail, size p1=%d,p2=%d\n",(int)p1.size(),(int)p2.size());
		return;
	}

	ptV src, dst;
	float obj_x1 = 0, obj_x2 = object_w;
	float obj_y1 = 0, obj_y2 = object_h;
	src.push_back(Point2f(obj_x1,obj_y1));
	src.push_back(Point2f(obj_x2,obj_y1));
	src.push_back(Point2f(obj_x2,obj_y2));
	src.push_back(Point2f(obj_x1,obj_y2));
	perspectiveTransform(src, dst, H);

	border_x1 = dst[0].x + scene_x;  border_y1 = dst[0].y + scene_y;
	border_x2 = dst[1].x + scene_x;  border_y2 = dst[1].y + scene_y;
	border_x3 = dst[2].x + scene_x;  border_y3 = dst[2].y + scene_y;
	border_x4 = dst[3].x + scene_x;  border_y4 = dst[3].y + scene_y;
//printf("src %f,%f  %f,%f  %f,%f  %f,%f\n",
// src[0].x,src[0].y, src[1].x,src[1].y, src[2].x,src[2].y, src[3].x,src[3].y);
//printf("dst %f,%f  %f,%f  %f,%f  %f,%f\n",
// dst[0].x,dst[0].y, dst[1].x,dst[1].y, dst[2].x,dst[2].y, dst[3].x,dst[3].y);
}

int FindObjectMain::process_buffer(VFrame **frame, int64_t start_position, double frame_rate)
{
	int prev_algorithm = config.algorithm;
	int prev_use_flann = config.use_flann;

	if( load_configuration() )
		init_border = 1;

	if( prev_algorithm != config.algorithm ||
	    prev_use_flann != config.use_flann ) {
		detector.release();
		matcher.release();
	}

	w = frame[0]->get_w();
	h = frame[0]->get_h();

	object_layer = config.object_layer;
	scene_layer = config.scene_layer;
	replace_layer = config.replace_layer;

	int max_layer = PluginClient::get_total_buffers() - 1;
	object_layer = bclip(config.object_layer, 0, max_layer);
	scene_layer = bclip(config.scene_layer, 0, max_layer);
	replace_layer = bclip(config.replace_layer, 0, max_layer);

	int cfg_w, cfg_h, cfg_x1, cfg_y1, cfg_x2, cfg_y2;
	cfg_w = (int)(config.object_w * w / 100);
	cfg_h = (int)(config.object_h * h / 100);
	cfg_x1 = (int)(config.object_x * w / 100 - cfg_w / 2);
	cfg_y1 = (int)(config.object_y * h / 100 - cfg_h / 2);
	cfg_x2 = cfg_x1 + cfg_w;
	cfg_y2 = cfg_y1 + cfg_h;
	bclamp(cfg_x1, 0, w);  object_x = cfg_x1;
	bclamp(cfg_y1, 0, h);  object_y = cfg_y1;
	bclamp(cfg_x2, 0, w);  object_w = cfg_x2 - cfg_x1;
	bclamp(cfg_y2, 0, h);  object_h = cfg_y2 - cfg_y1;

	cfg_w = (int)(config.scene_w * w / 100);
	cfg_h = (int)(config.scene_h * h / 100);
	cfg_x1 = (int)(config.scene_x * w / 100 - cfg_w / 2);
	cfg_y1 = (int)(config.scene_y * h / 100 - cfg_h / 2);
	cfg_x2 = cfg_x1 + cfg_w;
	cfg_y2 = cfg_y1 + cfg_h;
	bclamp(cfg_x1, 0, w);  scene_x = cfg_x1;
	bclamp(cfg_y1, 0, h);  scene_y = cfg_y1;
	bclamp(cfg_x2, 0, w);  scene_w = cfg_x2 - cfg_x1;
	bclamp(cfg_y2, 0, h);  scene_h = cfg_y2 - cfg_y1;

// Read in the input frames
	for( int i = 0; i < PluginClient::get_total_buffers(); i++ ) {
		read_frame(frame[i], i, start_position, frame_rate, 0);
	}

	object = frame[object_layer];
	scene = frame[scene_layer];
	replace = frame[replace_layer];

	border_x1 = obj_x1;  border_y1 = obj_y1;
	border_x2 = obj_x2;  border_y2 = obj_y2;
	border_x3 = obj_x3;  border_y3 = obj_y3;
	border_x4 = obj_x4;  border_y4 = obj_y4;

	if( scene_w > 0 && scene_h > 0 && object_w > 0 && object_h > 0 ) {
		process_match();
	}

	double w0 = init_border ? 1. : config.blend/100., w1 = 1. - w0;
	obj_x1 = border_x1*w0 + obj_x1*w1;  obj_y1 = border_y1*w0 + obj_y1*w1;
	obj_x2 = border_x2*w0 + obj_x2*w1;  obj_y2 = border_y2*w0 + obj_y2*w1;
	obj_x3 = border_x3*w0 + obj_x3*w1;  obj_y3 = border_y3*w0 + obj_y3*w1;
	obj_x4 = border_x4*w0 + obj_x4*w1;  obj_y4 = border_y4*w0 + obj_y4*w1;

// Replace object in the scene layer
	if( config.replace_object ) {
		int cpus1 = get_project_smp() + 1;
		if( !affine )
			affine = new AffineEngine(cpus1, cpus1);
		if( !overlayer )
			overlayer = new OverlayFrame(cpus1);

		VFrame *temp = new_temp(scene->get_w(), scene->get_h(), scene->get_color_model());
		temp->clear_frame();
		float sx = 100./w, sy = 100./h;
		float x1 = sx * obj_x1, y1 = sy * obj_y1;
		float x2 = sx * obj_x2, y2 = sy * obj_y2;
		float x3 = sx * obj_x3, y3 = sy * obj_y3;
		float x4 = sx * obj_x4, y4 = sy * obj_y4;
		affine->process(temp, replace, 0, AffineEngine::PERSPECTIVE,
			x1,y1, x2,y2, x3,y3, x4,y4, 1);
		overlayer->overlay(scene, temp,  0,0, w,h,  0,0, w,h,
			1, TRANSFER_NORMAL, NEAREST_NEIGHBOR);

	}

	if( config.draw_border ) {
		int wh = (w+h)>>8, ss = 1; while( wh>>=1 ) ss<<=1;
		scene->set_pixel_color(WHITE);  scene->set_stiple(ss*2);
		draw_line(scene, obj_x1, obj_y1, obj_x2, obj_y2);
		draw_line(scene, obj_x2, obj_y2, obj_x3, obj_y3);
		draw_line(scene, obj_x3, obj_y3, obj_x4, obj_y4);
		draw_line(scene, obj_x4, obj_y4, obj_x1, obj_y1);
	}

	if( config.draw_keypoints ) {
		scene->set_pixel_color(RED);  scene->set_stiple(0);
		for( int i=0,n=scn_keypts.size(); i<n; ++i ) {
			Point2f &pt = scn_keypts[i].pt;
			int r = scn_keypts[i].size * 1.2/9 * 2;
			int x = pt.x + scene_x, y = pt.y + scene_y;
			draw_circle(scene, x, y, r);
		}
	}

// Draw object outline in the object layer
	if( config.draw_object_border ) {
		int wh = (w+h)>>8, ss = 1; while( wh>>=1 ) ss<<=1;
		scene->set_pixel_color(YELLOW);  scene->set_stiple(ss*3);
		int x1 = object_x, x2 = x1 + object_w - 1;
		int y1 = object_y, y2 = y1 + object_h - 1;
		draw_rect(scene, x1, y1, x2, y2);
		scene->set_pixel_color(GREEN);  scene->set_stiple(0);
		x1 = scene_x, x2 = x1 + scene_w - 1;
		y1 = scene_y, y2 = y1 + scene_h - 1;
		draw_rect(scene, x1, y1, x2, y2);
	}

	scene->set_pixel_color(BLACK);
	scene->set_stiple(0);
	return 0;
}

