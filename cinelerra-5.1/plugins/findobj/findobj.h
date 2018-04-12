
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
 *
 */



// This is mainly a test for object tracking




#ifndef FINDOBJ_H
#define FINDOBJ_H

//#include "config.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "affine.inc"
#include "bccmodels.h"
#include "bchash.inc"
#include "filexml.inc"
#include "keyframe.inc"
#include "findobj.inc"
#include "overlayframe.inc"
#include "pluginvclient.h"
#include "vframe.inc"

#define Mutex CvMutex
#include "opencv2/core/types.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/flann/defines.h"
#include "opencv2/flann/params.h"
#undef Mutex

#include <vector>

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;
using namespace cvflann;

// enabled detectors
#define _SIFT
#define _SURF
#define _ORB
#define _AKAZE
#define _BRISK

// Limits of global range in percent
#define MIN_RADIUS 1
#define MAX_RADIUS 200

// Limits of block size in percent.
#define MIN_BLOCK 1
#define MAX_BLOCK 100

#define MIN_LAYER 0
#define MAX_LAYER 255

#define MIN_BLEND 1
#define MAX_BLEND 100

#define NO_ALGORITHM 0
#define ALGORITHM_SIFT  1
#define ALGORITHM_SURF  2
#define ALGORITHM_ORB   3
#define ALGORITHM_AKAZE 4
#define ALGORITHM_BRISK 5

class FindObjConfig
{
public:
	FindObjConfig();

	int equivalent(FindObjConfig &that);
	void copy_from(FindObjConfig &that);
	void interpolate(FindObjConfig &prev, FindObjConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);
	void boundaries();

	int algorithm, use_flann;
	int drag_object, drag_scene;
	float object_x, object_y, object_w, object_h;
	float scene_x, scene_y, scene_w, scene_h;

	int draw_keypoints;
	int draw_scene_border;
	int replace_object;
	int draw_object_border;

	int object_layer;
	int replace_layer;
	int scene_layer;
	int blend;
};

class FindObjMain : public PluginVClient
{
public:
	FindObjMain(PluginServer *server);
	~FindObjMain();

	int process_buffer(VFrame **frame, int64_t start_position, double frame_rate);
#ifdef _SIFT
	void set_sift();
#endif
#ifdef _SURF
	void set_surf();
#endif
#ifdef _ORB
	void set_orb();
#endif
#ifdef _AKAZE
	void set_akaze();
#endif
#ifdef _BRISK
	void set_brisk();
#endif
	void process_match();

	void draw_vectors(VFrame *frame);
	int is_multichannel();
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	PLUGIN_CLASS_MEMBERS2(FindObjConfig)

	AffineEngine *affine;
	OverlayFrame *overlayer;
	VFrame *object, *scene, *replace;

	static void draw_line(VFrame *vframe, int x1, int y1, int x2, int y2);
	static void draw_rect(VFrame *vframe, int x1, int y1, int x2, int y2);
	static void draw_circle(VFrame *vframe, int x, int y, int r);

	int object_x, object_y, object_w, object_h;
	int scene_x, scene_y, scene_w, scene_h;

	int w, h;
	int object_layer;
	int scene_layer;
	int replace_layer;

// Latest coordinates of object in scene
	int border_x1, border_y1;
	int border_x2, border_y2;
	int border_x3, border_y3;
	int border_x4, border_y4;
// Coordinates of object in scene with blending
	float obj_x1, obj_y1;
	float obj_x2, obj_y2;
	float obj_x3, obj_y3;
	float obj_x4, obj_y4;
	int init_border;

//opencv
	typedef vector<DMatch> DMatchV;
	typedef vector<DMatchV> DMatches;
	typedef vector<KeyPoint> KeyPointV;
	typedef vector<Point2f> ptV;

	BC_CModel cvmodel;
	Mat object_mat, scene_mat;
	Mat obj_descrs;  KeyPointV obj_keypts;
	Mat scn_descrs;  KeyPointV scn_keypts;
	DMatches pairs;

	static void to_mat(Mat &mat, int mcols, int mrows,
		VFrame *inp, int ix,int iy, BC_CModel mcolor_model);
	void detect(Mat &mat, KeyPointV &keypts, Mat &descrs);
	void match();
	void filter_matches(ptV &p1, ptV &p2, double ratio=0.75);

	Ptr<Feature2D> detector;
	Ptr<DescriptorMatcher> matcher;
	Ptr<DescriptorMatcher> flann_kdtree_matcher();
	Ptr<DescriptorMatcher> flann_lshidx_matcher();
	Ptr<DescriptorMatcher> bf_matcher_norm_l2();
	Ptr<DescriptorMatcher> bf_matcher_norm_hamming();
};

#endif
