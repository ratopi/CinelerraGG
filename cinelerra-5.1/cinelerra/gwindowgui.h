
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

#ifndef GWINDOWGUI_H
#define GWINDOWGUI_H

#include "automation.inc"
#include "condition.inc"
#include "colorpicker.h"
#include "guicast.h"
#include "gwindowgui.inc"
#include "mwindow.inc"

enum {
	NONAUTOTOGGLES_ASSETS,
	NONAUTOTOGGLES_TITLES,
	NONAUTOTOGGLES_TRANSITIONS,
	NONAUTOTOGGLES_PLUGIN_AUTOS,
	NONAUTOTOGGLES_BAR1,
	NONAUTOTOGGLES_BAR2,
	NONAUTOTOGGLES_COUNT
};

struct toggleinfo {
	int isauto, ref;
};

class GWindowGUI : public BC_Window
{
public:
	GWindowGUI(MWindow *mwindow, int w, int h);
	~GWindowGUI();
	static void calculate_extents(BC_WindowBase *gui, int *w, int *h);
	void create_objects();
	int translation_event();
	int close_event();
	int keypress_event();
	void start_color_thread(GWindowColorButton *color_button);
	void update_toggles(int use_lock);
	void update_mwindow();
	void load_defaults();
	void save_defaults();

	static const char *other_text[];
	static const char *auto_text[];
	static int auto_colors[];

	MWindow *mwindow;
	GWindowToggle *toggles[NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL];
	GWindowColorThread *color_thread;
};

class GWindowToggle : public BC_CheckBox
{
public:
	GWindowToggle(MWindow *mwindow, GWindowGUI *gui, int x, int y,
		const char *text, int color, toggleinfo *info);
	~GWindowToggle();

	int handle_event();
	void update();
	void update_gui(int color);

	static int* get_main_value(MWindow *mwindow, toggleinfo *info);

	int color;
	MWindow *mwindow;
	GWindowGUI *gui;
	toggleinfo *info;
	GWindowColorButton *color_button;
};

class GWindowColorButton : public BC_Button
{
public:
	GWindowColorButton(GWindowToggle *auto_toggle, int x, int y, int w);
	~GWindowColorButton();

	void set_color(int color);
	void update_gui(int color);
	int handle_event();

	int color;
	VFrame *vframes[3];
	GWindowToggle *auto_toggle;
};

class GWindowColorThread : public ColorPicker
{
public:
	GWindowColorThread(GWindowColorButton *color_button);
	~GWindowColorThread();
	void start(int color);
	int handle_new_color(int color, int alpha);
	void update_gui();
	void handle_done_event(int result);

	int color;
	GWindowColorButton *color_button;
	GWindowColorUpdate *color_update;
};

class GWindowColorUpdate : public Thread
{
public:
	GWindowColorUpdate(GWindowColorThread *color_thread);
	~GWindowColorUpdate();

	void start();
	void stop();
	void run();

	int done;
	Condition *update_lock;
	GWindowColorThread *color_thread;
};

#endif
