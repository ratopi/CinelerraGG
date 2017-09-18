
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


#ifndef __FINDOBJECTWINDOW_H__
#define __FINDOBJECTWINDOW_H__

#include "guicast.h"
#include "findobject.inc"

class FindObjectLayer;
class FindObjectScanFloat;
class FindObjectScanFloatText;
class FindObjectDrawBorder;
class FindObjectDrawKeypoints;
class FindObjectReplace;
class FindObjectDrawObjectBorder;
class FindObjectAlgorithm;
class FindObjectBlend;
class FindObjectWindow;

class FindObjectLayer : public BC_TumbleTextBox
{
public:
	FindObjectLayer(FindObjectMain *plugin, FindObjectWindow *gui,
		int x, int y, int *value);
	int handle_event();
	static int calculate_w(FindObjectWindow *gui);
	FindObjectMain *plugin;
	FindObjectWindow *gui;
	int *value;
};

class FindObjectScanFloat : public BC_FPot
{
public:
	FindObjectScanFloat(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y, float *value);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
	FindObjectScanFloatText *center_text;
	float *value;
};

class FindObjectScanFloatText : public BC_TextBox
{
public:
	FindObjectScanFloatText(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y, float *value);
	int handle_event();
	FindObjectWindow *gui;
	FindObjectMain *plugin;
	FindObjectScanFloat *center;
	float *value;
};


class FindObjectDrawBorder : public BC_CheckBox
{
public:
	FindObjectDrawBorder(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};

class FindObjectDrawKeypoints : public BC_CheckBox
{
public:
	FindObjectDrawKeypoints(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};

class FindObjectReplace : public BC_CheckBox
{
public:
	FindObjectReplace(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};

class FindObjectDrawObjectBorder : public BC_CheckBox
{
public:
	FindObjectDrawObjectBorder(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};

class FindObjectAlgorithm : public BC_PopupMenu
{
public:
	FindObjectAlgorithm(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	static int calculate_w(FindObjectWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};

class FindObjectUseFlann : public BC_CheckBox
{
public:
	FindObjectUseFlann(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y);
	int handle_event();
	FindObjectMain *plugin;
	FindObjectWindow *gui;
};

class FindObjectBlend : public BC_IPot
{
public:
	FindObjectBlend(FindObjectMain *plugin, int x, int y, int *value);
	int handle_event();
	FindObjectMain *plugin;
	int *value;
};

class FindObjectWindow : public PluginClientWindow
{
public:
	FindObjectWindow(FindObjectMain *plugin);
	~FindObjectWindow();
	void create_objects();

	FindObjectAlgorithm *algorithm;
	FindObjectUseFlann *use_flann;
	FindObjectScanFloat *object_x, *object_y, *object_w, *object_h;
	FindObjectScanFloatText *object_x_text, *object_y_text, *object_w_text, *object_h_text;
	FindObjectScanFloat *scene_x, *scene_y, *scene_w, *scene_h;
	FindObjectScanFloatText *scene_x_text, *scene_y_text, *scene_w_text, *scene_h_text;
	FindObjectDrawKeypoints *draw_keypoints;
	FindObjectDrawBorder *draw_border;
	FindObjectReplace *replace_object;
	FindObjectDrawObjectBorder *draw_object_border;
	FindObjectLayer *object_layer;
	FindObjectLayer *scene_layer;
	FindObjectLayer *replace_layer;
	FindObjectBlend *blend;
	FindObjectMain *plugin;
};

#endif
