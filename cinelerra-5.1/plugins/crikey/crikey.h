/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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



#ifndef EDGE_H
#define EDGE_H

#include "loadbalance.h"
#include "pluginvclient.h"

class CriKeyEngine;
class CriKey;

#define DRAW_ALPHA      0
#define DRAW_EDGE       1
#define DRAW_MASK       2
#define DRAW_MODES      3

#define KEY_SEARCH      0
#define KEY_SEARCH_ALL  1
#define KEY_POINT       2
#define KEY_MODES       3

class CriKeyConfig
{
public:
	CriKeyConfig();

	int equivalent(CriKeyConfig &that);
	void copy_from(CriKeyConfig &that);
	void interpolate(CriKeyConfig &prev, CriKeyConfig &next,
		long prev_frame, long next_frame, long current_frame);
	void limits();
	static void set_target(int is_yuv, int color, float *target);
	static void set_color(int is_yuv, float *target, int &color);

	int color;
	float threshold;
	int draw_mode, key_mode;
	float point_x, point_y;
	int drag;
};

class CriKeyPackage : public LoadPackage
{
public:
	CriKeyPackage() : LoadPackage() {}
	int y1, y2;
};

class CriKeyEngine : public LoadServer
{
public:
	CriKeyEngine(CriKey *plugin, int total_clients, int total_packages)
	 : LoadServer(total_clients, total_packages) { this->plugin = plugin; }
	~CriKeyEngine() {}

	void init_packages();
	LoadPackage* new_package();
	LoadClient* new_client();

	CriKey *plugin;
};

class CriKeyUnit : public LoadClient
{
public:
	CriKeyUnit(CriKeyEngine *server)
	 : LoadClient(server) { this->server = server; }
	~CriKeyUnit() {}

	float edge_detect(float *data, float max, int do_max);
	void process_package(LoadPackage *package);
	CriKeyEngine *server;
};


class CriKey : public PluginVClient
{
public:
	CriKey(PluginServer *server);
	~CriKey();
// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS2(CriKeyConfig)
	int is_realtime();
	void update_gui();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	void draw_alpha(VFrame *msk);
	void draw_mask(VFrame *msk);
	float diff_uint8(uint8_t *tp);
	float diff_float(uint8_t *tp);
	float (CriKey::*diff_pixel)(uint8_t *dp);
	void min_key(int &ix, int &iy);
	bool find_key(int &ix, int &iy, float threshold);
	static void set_target(int is_yuv, int color, float *target) {
		CriKeyConfig::set_target(is_yuv, color, target);
	}
	static void set_color(int is_yuv, float *target, int &color) {
		CriKeyConfig::set_color(is_yuv, target, color);
	}

	CriKeyEngine *engine;
	VFrame *src, *dst, *msk;
	int w, h, color_model, bpp, comp, is_yuv, is_float;

	void get_color(int x, int y);
	float target[3];
};

#endif
