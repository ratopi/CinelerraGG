
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
#include "bccolors.h"
#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "findobj.h"
#include "findobjwindow.h"
#include "mutex.h"
#include "overlayframe.h"
#include "plugin.h"
#include "pluginserver.h"
#include "track.h"

#include <errno.h>
#include <exception>
#include <unistd.h>

REGISTER_PLUGIN(FindObjMain)

FindObjConfig::FindObjConfig()
{
	reset();
}

void FindObjConfig::reset()
{
	algorithm = NO_ALGORITHM;
	use_flann = 1;
	draw_keypoints = 0;
	draw_scene_border = 0;
	replace_object = 0;
	draw_object_border = 0;
	draw_replace_border = 0;
	object_x = 50;  object_y = 50;
	object_w = 100; object_h = 100;
	drag_object = 0;
	scene_x = 50;   scene_y = 50;
	scene_w = 100;  scene_h = 100;
	drag_replace = 0;
	replace_x = 50;   replace_y = 50;
	replace_w = 100;  replace_h = 100;
	replace_dx = 0;   replace_dy = 0;
	drag_scene = 0;
	scene_layer = 0;
	object_layer = 1;
	replace_layer = 2;
	blend = 100;
}

void FindObjConfig::boundaries()
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

int FindObjConfig::equivalent(FindObjConfig &that)
{
	int result =
		algorithm == that.algorithm &&
		use_flann == that.use_flann &&
		draw_keypoints == that.draw_keypoints &&
		draw_scene_border == that.draw_scene_border &&
		replace_object == that.replace_object &&
		draw_object_border == that.draw_object_border &&
		draw_replace_border == that.draw_replace_border &&
		object_x == that.object_x && object_y == that.object_y &&
		object_w == that.object_w && object_h == that.object_h &&
		drag_object == that.drag_object &&
		scene_x == that.scene_x && scene_y == that.scene_y &&
		scene_w == that.scene_w && scene_h == that.scene_h &&
		drag_scene == that.drag_scene &&
		replace_x == that.replace_x && replace_y == that.replace_y &&
		replace_w == that.replace_w && replace_h == that.replace_h &&
		replace_dx == that.replace_dx && replace_dy == that.replace_dy &&
		drag_replace == that.drag_replace &&
		object_layer == that.object_layer &&
		replace_layer == that.replace_layer &&
		scene_layer == that.scene_layer &&
		blend == that.blend;
        return result;
}

void FindObjConfig::copy_from(FindObjConfig &that)
{
	algorithm = that.algorithm;
	use_flann = that.use_flann;
	draw_keypoints = that.draw_keypoints;
	draw_scene_border = that.draw_scene_border;
	replace_object = that.replace_object;
	draw_object_border = that.draw_object_border;
	draw_replace_border = that.draw_replace_border;
	object_x = that.object_x;  object_y = that.object_y;
	object_w = that.object_w;  object_h = that.object_h;
	drag_object = that.drag_object;
	scene_x = that.scene_x;    scene_y = that.scene_y;
	scene_w = that.scene_w;    scene_h = that.scene_h;
	drag_scene = that.drag_scene;
	replace_x = that.replace_x;   replace_y = that.replace_y;
	replace_w = that.replace_w;   replace_h = that.replace_h;
	replace_dx = that.replace_dx; replace_dy = that.replace_dy;
	drag_replace = that.drag_replace;
	object_layer = that.object_layer;
	replace_layer = that.replace_layer;
	scene_layer = that.scene_layer;
	blend = that.blend;
}

void FindObjConfig::interpolate(FindObjConfig &prev, FindObjConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}


FindObjMain::FindObjMain(PluginServer *server)
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

FindObjMain::~FindObjMain()
{
	delete affine;
	delete overlayer;
}

const char* FindObjMain::plugin_title() { return N_("FindObj"); }
int FindObjMain::is_realtime() { return 1; }
int FindObjMain::is_multichannel() { return 1; }

NEW_WINDOW_MACRO(FindObjMain, FindObjWindow)
LOAD_CONFIGURATION_MACRO(FindObjMain, FindObjConfig)

void FindObjMain::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	FindObjWindow *window = (FindObjWindow*)thread->window;
	window->lock_window("FindObjMain::update_gui");
	window->update_gui();
	window->flush();
	window->unlock_window();
}

void FindObjMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("FINDOBJ");
	output.tag.set_property("ALGORITHM", config.algorithm);
	output.tag.set_property("USE_FLANN", config.use_flann);
	output.tag.set_property("DRAG_OBJECT", config.drag_object);
	output.tag.set_property("OBJECT_X", config.object_x);
	output.tag.set_property("OBJECT_Y", config.object_y);
	output.tag.set_property("OBJECT_W", config.object_w);
	output.tag.set_property("OBJECT_H", config.object_h);
	output.tag.set_property("DRAG_SCENE", config.drag_scene);
	output.tag.set_property("SCENE_X", config.scene_x);
	output.tag.set_property("SCENE_Y", config.scene_y);
	output.tag.set_property("SCENE_W", config.scene_w);
	output.tag.set_property("SCENE_H", config.scene_h);
	output.tag.set_property("DRAG_REPLACE", config.drag_replace);
	output.tag.set_property("REPLACE_X", config.replace_x);
	output.tag.set_property("REPLACE_Y", config.replace_y);
	output.tag.set_property("REPLACE_W", config.replace_w);
	output.tag.set_property("REPLACE_H", config.replace_h);
	output.tag.set_property("REPLACE_DX", config.replace_dx);
	output.tag.set_property("REPLACE_DY", config.replace_dy);
	output.tag.set_property("DRAW_KEYPOINTS", config.draw_keypoints);
	output.tag.set_property("DRAW_SCENE_BORDER", config.draw_scene_border);
	output.tag.set_property("REPLACE_OBJECT", config.replace_object);
	output.tag.set_property("DRAW_OBJECT_BORDER", config.draw_object_border);
	output.tag.set_property("DRAW_REPLACE_BORDER", config.draw_replace_border);
	output.tag.set_property("OBJECT_LAYER", config.object_layer);
	output.tag.set_property("REPLACE_LAYER", config.replace_layer);
	output.tag.set_property("SCENE_LAYER", config.scene_layer);
	output.tag.set_property("BLEND", config.blend);
	output.append_tag();
	output.tag.set_title("/FINDOBJ");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void FindObjMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->get_data(), strlen(keyframe->get_data()));

	int result = 0;

	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("FINDOBJ") ) {
			config.algorithm = input.tag.get_property("ALGORITHM", config.algorithm);
			config.use_flann = input.tag.get_property("USE_FLANN", config.use_flann);
			config.object_x = input.tag.get_property("OBJECT_X", config.object_x);
			config.object_y = input.tag.get_property("OBJECT_Y", config.object_y);
			config.object_w = input.tag.get_property("OBJECT_W", config.object_w);
			config.object_h = input.tag.get_property("OBJECT_H", config.object_h);
			config.drag_object = input.tag.get_property("DRAG_OBJECT", config.drag_object);
			config.scene_x = input.tag.get_property("SCENE_X", config.scene_x);
			config.scene_y = input.tag.get_property("SCENE_Y", config.scene_y);
			config.scene_w = input.tag.get_property("SCENE_W", config.scene_w);
			config.scene_h = input.tag.get_property("SCENE_H", config.scene_h);
			config.drag_scene = input.tag.get_property("DRAG_SCENE", config.drag_scene);
			config.replace_x = input.tag.get_property("REPLACE_X", config.replace_x);
			config.replace_y = input.tag.get_property("REPLACE_Y", config.replace_y);
			config.replace_w = input.tag.get_property("REPLACE_W", config.replace_w);
			config.replace_h = input.tag.get_property("REPLACE_H", config.replace_h);
			config.replace_dx = input.tag.get_property("REPLACE_DX", config.replace_dx);
			config.replace_dy = input.tag.get_property("REPLACE_DY", config.replace_dy);
			config.drag_replace = input.tag.get_property("DRAG_REPLACE", config.drag_replace);
			config.draw_keypoints = input.tag.get_property("DRAW_KEYPOINTS", config.draw_keypoints);
			config.draw_scene_border = input.tag.get_property("DRAW_SCENE_BORDER", config.draw_scene_border);
			config.replace_object = input.tag.get_property("REPLACE_OBJECT", config.replace_object);
			config.draw_object_border = input.tag.get_property("DRAW_OBJECT_BORDER", config.draw_object_border);
			config.draw_replace_border = input.tag.get_property("DRAW_REPLACE_BORDER", config.draw_replace_border);
			config.object_layer = input.tag.get_property("OBJECT_LAYER", config.object_layer);
			config.replace_layer = input.tag.get_property("REPLACE_LAYER", config.replace_layer);
			config.scene_layer = input.tag.get_property("SCENE_LAYER", config.scene_layer);
			config.blend = input.tag.get_property("BLEND", config.blend);
		}
	}

	config.boundaries();
}

void FindObjMain::draw_line(VFrame *vframe, int x1, int y1, int x2, int y2)
{
	vframe->draw_line(x1, y1, x2, y2);
}

void FindObjMain::draw_rect(VFrame *vframe, int x1, int y1, int x2, int y2)
{
	--x2;  --y2;
	draw_line(vframe, x1, y1, x2, y1);
	draw_line(vframe, x2, y1, x2, y2);
	draw_line(vframe, x2, y2, x1, y2);
	draw_line(vframe, x1, y2, x1, y1);
}

void FindObjMain::draw_circle(VFrame *vframe, int x, int y, int r)
{
	int x1 = x-r, x2 = x+r;
	int y1 = y-r, y2 = y+r;
	vframe->draw_smooth(x1,y, x1,y1, x,y1);
	vframe->draw_smooth(x,y1, x2,y1, x2,y);
	vframe->draw_smooth(x2,y, x2,y2, x,y2);
	vframe->draw_smooth(x,y2, x1,y2, x1,y);
}

void FindObjMain::filter_matches(ptV &p1, ptV &p2, double ratio)
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

void FindObjMain::to_mat(Mat &mat, int mcols, int mrows,
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

void FindObjMain::detect(Mat &mat, KeyPointV &keypts,Mat &descrs)
{
	keypts.clear();
	descrs.release();
	try {
		detector->detectAndCompute(mat, noArray(), keypts, descrs);
	} catch(std::exception e) { printf(_("detector exception: %s\n"), e.what()); }
}

void FindObjMain::match()
{
	pairs.clear();
	try {
		matcher->knnMatch(obj_descrs, scn_descrs, pairs, 2);
	} catch(std::exception e) { printf(_("match execption: %s\n"), e.what()); }
}

Ptr<DescriptorMatcher> FindObjMain::flann_kdtree_matcher()
{ // trees=5
	const Ptr<flann::IndexParams>& indexParams =
		makePtr<flann::KDTreeIndexParams>(5);
	const Ptr<flann::SearchParams>& searchParams =
		makePtr<flann::SearchParams>();
	return makePtr<FlannBasedMatcher>(indexParams, searchParams);
}
Ptr<DescriptorMatcher> FindObjMain::flann_lshidx_matcher()
{ // table_number = 6#12, key_size = 12#20, multi_probe_level = 1#2
	const Ptr<flann::IndexParams>& indexParams =
		makePtr<flann::LshIndexParams>(6, 12, 1);
	const Ptr<flann::SearchParams>& searchParams =
		makePtr<flann::SearchParams>();
	return makePtr<FlannBasedMatcher>(indexParams, searchParams);
}
Ptr<DescriptorMatcher> FindObjMain::bf_matcher_norm_l2()
{
	return BFMatcher::create(NORM_L2);
}
Ptr<DescriptorMatcher> FindObjMain::bf_matcher_norm_hamming()
{
	return BFMatcher::create(NORM_HAMMING);
}

#ifdef _SIFT
void FindObjMain::set_sift()
{
	cvmodel = BC_GREY8;
	detector = SIFT::create();
	matcher = config.use_flann ?
		flann_kdtree_matcher() : bf_matcher_norm_l2();
}
#endif
#ifdef _SURF
void FindObjMain::set_surf()
{
	cvmodel = BC_GREY8;
	detector = SURF::create(800);
	matcher = config.use_flann ?
		flann_kdtree_matcher() : bf_matcher_norm_l2();
}
#endif
#ifdef _ORB
void FindObjMain::set_orb()
{
	cvmodel = BC_GREY8;
	detector = ORB::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif
#ifdef _AKAZE
void FindObjMain::set_akaze()
{
	cvmodel = BC_GREY8;
	detector = AKAZE::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif
#ifdef _BRISK
void FindObjMain::set_brisk()
{
	cvmodel = BC_GREY8;
	detector = BRISK::create();
	matcher = config.use_flann ?
		flann_lshidx_matcher() : bf_matcher_norm_hamming();
}
#endif

void FindObjMain::process_match()
{
	if( config.algorithm == NO_ALGORITHM ) return;
	if( !config.replace_object &&
	    !config.draw_scene_border &&
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

	float dx = scene_x + replace_dx;
	float dy = scene_y + replace_dy;
	border_x1 = dst[0].x + dx;  border_y1 = dst[0].y + dy;
	border_x2 = dst[1].x + dx;  border_y2 = dst[1].y + dy;
	border_x3 = dst[2].x + dx;  border_y3 = dst[2].y + dy;
	border_x4 = dst[3].x + dx;  border_y4 = dst[3].y + dy;
//printf("src %f,%f  %f,%f  %f,%f  %f,%f\n",
// src[0].x,src[0].y, src[1].x,src[1].y, src[2].x,src[2].y, src[3].x,src[3].y);
//printf("dst %f,%f  %f,%f  %f,%f  %f,%f\n",
// dst[0].x,dst[0].y, dst[1].x,dst[1].y, dst[2].x,dst[2].y, dst[3].x,dst[3].y);
}

int FindObjMain::process_buffer(VFrame **frame, int64_t start_position, double frame_rate)
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

	object_layer = config.object_layer;
	scene_layer = config.scene_layer;
	replace_layer = config.replace_layer;
	Track *track = server->plugin->track;
	w = track->track_w;
	h = track->track_h;

	int max_layer = PluginClient::get_total_buffers() - 1;
	object_layer = bclip(config.object_layer, 0, max_layer);
	scene_layer = bclip(config.scene_layer, 0, max_layer);
	replace_layer = bclip(config.replace_layer, 0, max_layer);

	int cfg_w = (int)(w * config.object_w / 100.);
	int cfg_h = (int)(h * config.object_h / 100.);
	int cfg_x1 = (int)(w * config.object_x / 100. - cfg_w / 2);
	int cfg_y1 = (int)(h * config.object_y / 100. - cfg_h / 2);
	int cfg_x2 = cfg_x1 + cfg_w;
	int cfg_y2 = cfg_y1 + cfg_h;
	bclamp(cfg_x1, 0, w);  object_x = cfg_x1;
	bclamp(cfg_y1, 0, h);  object_y = cfg_y1;
	bclamp(cfg_x2, 0, w);  object_w = cfg_x2 - cfg_x1;
	bclamp(cfg_y2, 0, h);  object_h = cfg_y2 - cfg_y1;

	cfg_w = (int)(w * config.scene_w / 100.);
	cfg_h = (int)(h * config.scene_h / 100.);
	cfg_x1 = (int)(w * config.scene_x / 100. - cfg_w / 2);
	cfg_y1 = (int)(h * config.scene_y / 100. - cfg_h / 2);
	cfg_x2 = cfg_x1 + cfg_w;
	cfg_y2 = cfg_y1 + cfg_h;
	bclamp(cfg_x1, 0, w);  scene_x = cfg_x1;
	bclamp(cfg_y1, 0, h);  scene_y = cfg_y1;
	bclamp(cfg_x2, 0, w);  scene_w = cfg_x2 - cfg_x1;
	bclamp(cfg_y2, 0, h);  scene_h = cfg_y2 - cfg_y1;

	cfg_w = (int)(w * config.replace_w / 100.);
	cfg_h = (int)(h * config.replace_h / 100.);
	cfg_x1 = (int)(w * config.replace_x / 100. - cfg_w / 2);
	cfg_y1 = (int)(h * config.replace_y / 100. - cfg_h / 2);
	cfg_x2 = cfg_x1 + cfg_w;
	cfg_y2 = cfg_y1 + cfg_h;
	bclamp(cfg_x1, 0, w);  replace_x = cfg_x1;
	bclamp(cfg_y1, 0, h);  replace_y = cfg_y1;
	bclamp(cfg_x2, 0, w);  replace_w = cfg_x2 - cfg_x1;
	bclamp(cfg_y2, 0, h);  replace_h = cfg_y2 - cfg_y1;

	int cfg_dx = (int)(w * config.replace_dx / 100.);
	int cfg_dy = (int)(h * config.replace_dy / 100.);
	bclamp(cfg_dx, -h, h);  replace_dx = cfg_dx;
	bclamp(cfg_dy, -w, w);  replace_dy = cfg_dy;

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

	double w0 = init_border ? (init_border=0, 1.) : config.blend/100., w1 = 1. - w0;
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
		VFrame *temp = new_temp(w, h, scene->get_color_model());
		temp->clear_frame();
		affine->set_in_viewport(replace_x, replace_y, replace_w, replace_h);
		int x1 = obj_x1, x2 = obj_x2, x3 = obj_x3, x4 = obj_x4;
		int y1 = obj_y1, y2 = obj_y2, y3 = obj_y3, y4 = obj_y4;
		bclamp(x1, 0, w);  bclamp(x2, 0, w);  bclamp(x3, 0, w);  bclamp(x4, 0, w);
		bclamp(y1, 0, h);  bclamp(y2, 0, h);  bclamp(y3, 0, h);  bclamp(y4, 0, h);
		affine->set_matrix(
			replace_x, replace_y, replace_x+replace_w, replace_y+replace_h,
			x1,y1, x2,y2, x3,y3, x4,y4);
		affine->process(temp, replace, 0,
			AffineEngine::TRANSFORM, 0,0, 100,0, 100,100, 0,100, 1);
		overlayer->overlay(scene, temp,  0,0, w,h,  0,0, w,h,
			1, TRANSFER_NORMAL, NEAREST_NEIGHBOR);

	}

	if( config.draw_scene_border ) {
		int wh = (w+h)>>8, ss = 1; while( wh>>=1 ) ss<<=1;
		scene->set_pixel_color(WHITE);  scene->set_stiple(ss*2);
		draw_rect(scene, scene_x, scene_y, scene_x+scene_w, scene_y+scene_h);
	}
	if( config.draw_object_border ) {
		int wh = (w+h)>>8, ss = 1; while( wh>>=1 ) ss<<=1;
		scene->set_pixel_color(YELLOW);  scene->set_stiple(ss*3);
		draw_rect(scene, object_x, object_y, object_x+object_w, object_y+object_h);
	}
	if( config.draw_replace_border ) {
		int wh = (w+h)>>8, ss = 1; while( wh>>=1 ) ss<<=1;
		scene->set_pixel_color(GREEN);  scene->set_stiple(ss*3);
		draw_rect(scene, replace_x, replace_y, replace_x+replace_w, replace_y+replace_h);
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

	if( gui_open() ) {
		if( config.drag_scene ) {
			scene->set_pixel_color(WHITE);
			DragCheckBox::draw_boundary(scene, scene_x, scene_y, scene_w, scene_h);
		}
		if( config.drag_object ) {
			scene->set_pixel_color(YELLOW);
			DragCheckBox::draw_boundary(scene, object_x, object_y, object_w, object_h);
		}
		if( config.drag_replace ) {
			scene->set_pixel_color(GREEN);
			DragCheckBox::draw_boundary(scene, replace_x, replace_y, replace_w, replace_h);
		}
	}

	scene->set_pixel_color(BLACK);
	scene->set_stiple(0);
	return 0;
}

