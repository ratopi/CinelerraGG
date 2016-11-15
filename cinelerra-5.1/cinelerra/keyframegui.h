
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef KEYFRAMEGUI_H
#define KEYFRAMEGUI_H


#include "bcdialog.h"
#include "guicast.h"

class KeyFrameWindow;


#define KEYFRAME_COLUMNS 2
// Enable editing of detailed keyframe parameters.
class KeyFrameThread : public BC_DialogThread
{
public:
	KeyFrameThread(MWindow *mwindow);
	~KeyFrameThread();


	BC_Window* new_gui();
	void start_window(Plugin *plugin, KeyFrame *keyframe);
	void handle_done_event(int result);
	void handle_close_event(int result);
	void update_values();
	void save_value(char *value);
	void apply_value();
	void update_gui(int update_value_text = 1);

	ArrayList<BC_ListBoxItem*> *keyframe_data;
	Plugin *plugin;
	KeyFrame *keyframe;
	MWindow *mwindow;
	char window_title[BCTEXTLEN];
	char plugin_title[BCTEXTLEN];
	char *column_titles[KEYFRAME_COLUMNS];
	int column_width[KEYFRAME_COLUMNS];
};

class KeyFrameList : public BC_ListBox
{
public:
	KeyFrameList(KeyFrameThread *thread,
		KeyFrameWindow *window, int x, int y, int w, int h);
	int selection_changed();
	int handle_event();
	int column_resize_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFrameParamList : public BC_ListBox
{
public:
	KeyFrameParamList(KeyFrameThread *thread,
		KeyFrameWindow *window, int x, int y, int w, int h);
	int selection_changed();
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFrameValue : public BC_TextBox
{
public:
	KeyFrameValue(KeyFrameThread *thread,
		KeyFrameWindow *window, int x, int y, int w);
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFrameAll : public BC_CheckBox
{
public:
	KeyFrameAll(KeyFrameThread *thread,
		KeyFrameWindow *window, int x, int y);
	int handle_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFrameParamsOK : public BC_OKButton
{
public:
	KeyFrameParamsOK(KeyFrameThread *thread,
		KeyFrameWindow *window);
	int keypress_event();
	KeyFrameThread *thread;
	KeyFrameWindow *window;
};

class KeyFrameWindow : public BC_Window
{
public:
	KeyFrameWindow(MWindow *mwindow,
		KeyFrameThread *thread, int x, int y, char *title);
	void create_objects();
	int resize_event(int w, int h);

// List of parameters, values, & whether the parameter is defined by the current keyframe.
	KeyFrameList *keyframe_list;
// The text area of the plugin
//	KeyFrameText *keyframe_text;
// Value text of the current parameter
	KeyFrameValue *value_text;
	KeyFrameAll *all_toggle;
	BC_Title *title1, *title3;

	MWindow *mwindow;
	KeyFrameThread *thread;
};

#endif
