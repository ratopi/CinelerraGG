/*
 * CINELERRA
 * Copyright (C) 2014 Adam Williams <broadcast at earthling dot net>
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

#include "automation.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "crikey.h"
#include "crikeywindow.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"

#define COLOR_W 50
#define COLOR_H 30

CriKeyNum::CriKeyNum(CriKeyWindow *gui, int x, int y, float &output)
 : BC_TumbleTextBox(gui, output, -32767.0f, 32767.0f, x, y, 80)
{
	this->gui = gui;
	this->output = &output;
	set_increment(1);
	set_precision(1);
}

CriKeyNum::~CriKeyNum()
{
}

int CriKeyNum::handle_event()
{
	*output = atof(get_text());
	gui->plugin->send_configure_change();
	return 1;
}

int CriKeyDrawModeItem::handle_event()
{
	((CriKeyDrawMode *)get_popup_menu())->update(id);
	return 1;
}
CriKeyDrawMode::CriKeyDrawMode(CriKeyWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, 100, "", 1)
{
	this->gui = gui;
	draw_modes[DRAW_ALPHA]     = _("Alpha");
	draw_modes[DRAW_EDGE]      = _("Edge");
	draw_modes[DRAW_MASK]      = _("Mask");
	mode = -1;
}
void CriKeyDrawMode::create_objects()
{
	for( int i=0; i<DRAW_MODES; ++i )
		add_item(new CriKeyDrawModeItem(draw_modes[i], i));
	update(gui->plugin->config.draw_mode, 0);
}
void CriKeyDrawMode::update(int mode, int send)
{
	if( this->mode == mode ) return;
	this->mode = mode;
	set_text(draw_modes[mode]);
	gui->plugin->config.draw_mode = mode;
	if( send ) gui->plugin->send_configure_change();
}

int CriKeyKeyModeItem::handle_event()
{
	((CriKeyKeyMode *)get_popup_menu())->update(id);
	return 1;
}
CriKeyKeyMode::CriKeyKeyMode(CriKeyWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, 100, "", 1)
{
	this->gui = gui;
	key_modes[KEY_SEARCH]      = _("Search");
	key_modes[KEY_SEARCH_ALL]  = _("Search all");
	key_modes[KEY_POINT]       = _("Point");
	this->mode = -1;
}
void CriKeyKeyMode::create_objects()
{
	for( int i=0; i<KEY_MODES; ++i )
		add_item(new CriKeyKeyModeItem(key_modes[i], i));
	update(gui->plugin->config.key_mode, 0);
}
void CriKeyKeyMode::update(int mode, int send)
{
	if( this->mode == mode ) return;
	this->mode = mode;
	set_text(key_modes[mode]);
	gui->draw_key(mode);
	if( send ) gui->plugin->send_configure_change();
}

CriKeyColorButton::CriKeyColorButton(CriKeyWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Color"))
{
	this->gui = gui;
}
int CriKeyColorButton::handle_event()
{
	gui->start_color_thread();
	return 1;
}

CriKeyColorPicker::CriKeyColorPicker(CriKeyColorButton *color_button)
 : ColorPicker(0, _("Color"))
{
	this->color_button = color_button;
}

void CriKeyColorPicker::start(int color)
{
	start_window(this->color = color, 0, 1);
}

void CriKeyColorPicker::handle_done_event(int result)
{
	if( !result ) {
		CriKeyWindow *gui = color_button->gui;
		gui->lock_window("CriKeyColorPicker::handle_done_event");
		gui->update_color(color);
		gui->plugin->config.color = color;
		gui->plugin->send_configure_change();
		gui->unlock_window();
	}
}

int CriKeyColorPicker::handle_new_color(int color, int alpha)
{
	CriKeyWindow *gui = color_button->gui;
	gui->lock_window("CriKeyColorPicker::handle_new_color");
	gui->update_color(this->color = color);
	gui->flush();
	gui->plugin->config.color = color;
	gui->plugin->send_configure_change();
	gui->unlock_window();
	return 1;
}


void CriKeyWindow::start_color_thread()
{
	unlock_window();
	delete color_picker;
	color_picker = new CriKeyColorPicker(color_button);
	color_picker->start(plugin->config.color);
	lock_window("CriKeyWindow::start_color_thread");
}


CriKeyWindow::CriKeyWindow(CriKey *plugin)
 : PluginClientWindow(plugin, 320, 220, 320, 220, 0)
{
	this->plugin = plugin;
	this->color_button = 0;
	this->color_picker = 0;
	this->title_x = 0;  this->point_x = 0;
	this->title_y = 0;  this->point_y = 0;
	this->dragging = 0; this->drag = 0;
}

CriKeyWindow::~CriKeyWindow()
{
	delete color_picker;
}

void CriKeyWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = plugin->get_theme()->widget_border;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Threshold:")));
	y += title->get_h() + margin;
	add_subwindow(threshold = new CriKeyThreshold(this, x, y, get_w() - x * 2));
	y += threshold->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y+5, _("Draw mode:")));
	int x1 = x + title->get_w() + 10 + margin;
	add_subwindow(draw_mode = new CriKeyDrawMode(this, x1, y));
	draw_mode->create_objects();
	y += draw_mode->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y+5, _("Key mode:")));
	add_subwindow(key_mode = new CriKeyKeyMode(this, x1, y));
	y += key_mode->get_h() + margin;
	key_x = x + 10 + margin;  key_y = y + 10 + margin;
	key_mode->create_objects();

        if( plugin->config.drag )
                grab(plugin->server->mwindow->cwindow->gui);
	show_window(1);
}

int CriKeyWindow::grab_event(XEvent *event)
{
	if( key_mode->mode != KEY_POINT ) return 0;

	MWindow *mwindow = plugin->server->mwindow;
	CWindowGUI *cwindow_gui = mwindow->cwindow->gui;
	CWindowCanvas *canvas = cwindow_gui->canvas;
	int cx, cy;  canvas->get_canvas()->get_relative_cursor_xy(cx, cy);
	if( cx < mwindow->theme->ccanvas_x ) return 0;
	if( cx >= mwindow->theme->ccanvas_x+mwindow->theme->ccanvas_w ) return 0;
	if( cy < mwindow->theme->ccanvas_y ) return 0;
	if( cy >= mwindow->theme->ccanvas_y+mwindow->theme->ccanvas_h ) return 0;

	switch( event->type ) {
	case ButtonPress:
		if( dragging ) return 0;
		dragging = 1;
		break;
	case ButtonRelease:
		if( !dragging ) return 0;
		dragging = 0;
		return 1;
	case MotionNotify:
		if( dragging ) break;
	default:
		return 0;
	}

	float cursor_x = cx, cursor_y = cy;
	canvas->canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
	int64_t position = plugin->get_source_position();
	float projector_x, projector_y, projector_z;
	Track *track = plugin->server->plugin->track;
	int track_w = track->track_w, track_h = track->track_h;
	track->automation->get_projector(
		&projector_x, &projector_y, &projector_z,
		position, PLAY_FORWARD);
	projector_x += mwindow->edl->session->output_w / 2;
	projector_y += mwindow->edl->session->output_h / 2;
	float output_x = (cursor_x - projector_x) / projector_z + track_w / 2;
	float output_y = (cursor_y - projector_y) / projector_z + track_h / 2;
	point_x->update((int64_t)(plugin->config.point_x = output_x));
	point_y->update((int64_t)(plugin->config.point_y = output_y));
	plugin->send_configure_change();
	return 1;
}

void CriKeyWindow::done_event(int result)
{
        ungrab(client->server->mwindow->cwindow->gui);
	if( color_picker ) color_picker->close_window();
}

void CriKeyWindow::draw_key(int mode)
{
	int margin = plugin->get_theme()->widget_border;
	int x = key_x, y = key_y;
	delete color_picker;  color_picker = 0;
	delete color_button;  color_button = 0;
	delete title_x;  title_x = 0;
	delete point_x;  point_x = 0;
	delete title_y;  title_y = 0;
	delete point_y;  point_y = 0;
	delete drag;     drag = 0;

	clear_box(x,y, get_w()-x,get_h()-y);
	flash(x,y, get_w()-x,get_h()-y);

	switch( mode ) {
	case KEY_SEARCH:
	case KEY_SEARCH_ALL:
		add_subwindow(color_button = new CriKeyColorButton(this, x, y));
		x += color_button->get_w() + margin;
		color_x = x;  color_y = y;
		update_color(plugin->config.color);
		break;
	case KEY_POINT:
		add_subwindow(title_x = new BC_Title(x, y, _("X:")));
		int x1 = x + title_x->get_w() + margin, y1 = y;
		point_x = new CriKeyNum(this, x1, y, plugin->config.point_x);
		point_x->create_objects();
		y += point_x->get_h() + margin;
		add_subwindow(title_y = new BC_Title(x, y, _("Y:")));
		point_y = new CriKeyNum(this, x1, y, plugin->config.point_y);
		point_y->create_objects();
		x1 += point_x->get_w() + margin;
		add_subwindow(drag = new CriKeyDrag(this, x1, y1));
		break;
	}
	plugin->config.key_mode = mode;
	show_window(1);
}

void CriKeyWindow::update_color(int color)
{
	set_color(color);
	draw_box(color_x, color_y, COLOR_W, COLOR_H);
	set_color(BLACK);
	draw_rectangle(color_x, color_y, COLOR_W, COLOR_H);
	flash(color_x, color_y, COLOR_W, COLOR_H);
}

void CriKeyWindow::update_gui()
{
	threshold->update(plugin->config.threshold);
	draw_mode->update(plugin->config.draw_mode);
	key_mode->update(plugin->config.key_mode);
	switch( plugin->config.key_mode ) {
	case KEY_POINT:
		point_x->update(plugin->config.point_x);
		point_y->update(plugin->config.point_y);
		break;
	case KEY_SEARCH:
	case KEY_SEARCH_ALL:
		update_color(plugin->config.color);
		break;
	}
}


CriKeyThreshold::CriKeyThreshold(CriKeyWindow *gui, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, 0, 1, gui->plugin->config.threshold, 0)
{
	this->gui = gui;
	set_precision(0.005);
}

int CriKeyThreshold::handle_event()
{
	gui->plugin->config.threshold = get_value();
	gui->plugin->send_configure_change();
	return 1;
}


CriKeyDrag::CriKeyDrag(CriKeyWindow *gui, int x, int y)
 : BC_CheckBox(x, y, gui->plugin->config.drag, _("Drag"))
{
	this->gui = gui;
}
int CriKeyDrag::handle_event()
{
        int value = get_value();
        gui->plugin->config.drag = value;
	CWindowGUI *cwindow_gui = gui->plugin->server->mwindow->cwindow->gui;
        if( value )
                gui->grab(cwindow_gui);
        else
                gui->ungrab(cwindow_gui);
        gui->plugin->send_configure_change();
        return 1;
}

