/*
 * CINELERRA
 * Copyright (C) 2008-2015 Adam Williams <broadcast at earthling dot net>
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

#ifndef __CRIKEYWINDOW_H__
#define __CRIKEYWINDOW_H__

#include "guicast.h"
#include "colorpicker.h"

class CriKey;
class CriKeyWindow;
class CriKeyNum;
class CriKeyColorButton;
class CriKeyColorPicker;
class CriKeyDrawMode;
class CriKeyDrawModeItem;
class CriKeyKeyMode;
class CriKeyKeyModeItem;
class CriKeyThreshold;

class CriKeyNum : public BC_TumbleTextBox
{
public:
	CriKeyWindow *gui;
	float *output;
	int handle_event();

	CriKeyNum(CriKeyWindow *gui, int x, int y, float &output);
	~CriKeyNum();
};

class CriKeyColorButton : public BC_GenericButton
{
public:
	CriKeyColorButton(CriKeyWindow *gui, int x, int y);

	int handle_event();
	CriKeyWindow *gui;
};

class CriKeyColorPicker : public ColorPicker
{
public:
	CriKeyColorPicker(CriKeyColorButton *color_button);

	void start(int color);
	int handle_new_color(int color, int alpha);
	void handle_done_event(int result);

	CriKeyColorButton *color_button;
	int color;
};

class CriKeyDrawMode : public BC_PopupMenu
{
	const char *draw_modes[DRAW_MODES];
public:
	CriKeyDrawMode(CriKeyWindow *gui, int x, int y);

	void create_objects();
	void update(int mode);
	CriKeyWindow *gui;
	int mode;
};
class CriKeyDrawModeItem : public BC_MenuItem
{
public:
	CriKeyDrawModeItem(const char *txt, int id)
	: BC_MenuItem(txt) { this->id = id; }

	int handle_event();
	CriKeyWindow *gui;
	int id;
};

class CriKeyKeyMode : public BC_PopupMenu
{
	const char *key_modes[KEY_MODES];
public:
	CriKeyKeyMode(CriKeyWindow *gui, int x, int y);

	void create_objects();
	void update(int mode);
	CriKeyWindow *gui;
	int mode;
};
class CriKeyKeyModeItem : public BC_MenuItem
{
public:
	CriKeyKeyModeItem(const char *text, int id)
	: BC_MenuItem(text) { this->id = id; }

	int handle_event();
	CriKeyWindow *gui;
	int id;
};

class CriKeyThreshold : public BC_FSlider
{
public:
	CriKeyThreshold(CriKeyWindow *gui, int x, int y, int w);
	int handle_event();
	CriKeyWindow *gui;
};

class CriKeyDrag : public BC_CheckBox
{
public:
	CriKeyDrag(CriKeyWindow *gui, int x, int y);

	int handle_event();
	CriKeyWindow *gui;
};

class CriKeyWindow : public PluginClientWindow
{
public:
	CriKeyWindow(CriKey *plugin);
	~CriKeyWindow();

	void create_objects();
	void draw_key(int mode);
	void update_color(int color);
	void update_gui();
	void start_color_thread();
	int grab_event(XEvent *event);
	void done_event(int result);

	CriKey *plugin;
	CriKeyThreshold *threshold;
	CriKeyDrawMode *draw_mode;
	CriKeyKeyMode *key_mode;

	CriKeyColorButton *color_button;
	CriKeyColorPicker *color_picker;
	int color_x, color_y, key_x, key_y;
	BC_Title *title_x, *title_y;
	CriKeyNum *point_x, *point_y;
	int dragging;
	CriKeyDrag *drag;
};

#endif

