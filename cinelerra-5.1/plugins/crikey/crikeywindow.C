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
#include "cstrdup.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"

#define COLOR_W 50
#define COLOR_H 30

CriKeyNum::CriKeyNum(CriKeyWindow *gui, int x, int y, float output)
 : BC_TumbleTextBox(gui, output, -32767.0f, 32767.0f, x, y, 120)
{
	this->gui = gui;
	set_increment(1);
	set_precision(1);
}

CriKeyNum::~CriKeyNum()
{
}

int CriKeyPointX::handle_event()
{
	if( !CriKeyNum::handle_event() ) return 0;
	CriKeyPoints *points = gui->points;
	int hot_point = points->get_selection_number(0, 0);
	if( hot_point >= 0 && hot_point < gui->plugin->config.points.size() ) {
		float v = atof(get_text());
		gui->plugin->config.points[hot_point]->x = v;
		points->set_point(hot_point, PT_X, v);
	}
	points->update_list();
	gui->plugin->send_configure_change();
	return 1;
}
int CriKeyPointY::handle_event()
{
	if( !CriKeyNum::handle_event() ) return 0;
	CriKeyPoints *points = gui->points;
	int hot_point = points->get_selection_number(0, 0);
	if( hot_point >= 0 && hot_point < gui->plugin->config.points.size() ) {
		float v = atof(get_text());
		gui->plugin->config.points[hot_point]->y = v;
		points->set_point(hot_point, PT_Y, v);
	}
	points->update_list();
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
	orig_color = this->color = color;
	start_window(color, 0, 1);
}

void CriKeyColorPicker::handle_done_event(int result)
{
	if( result ) color = orig_color;
	CriKeyWindow *gui = color_button->gui;
	gui->lock_window("CriKeyColorPicker::handle_done_event");
	gui->update_color(color);
	gui->plugin->config.color = color;
	gui->plugin->send_configure_change();
	gui->unlock_window();
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
 : PluginClientWindow(plugin, 380, 360, 380, 360, 0)
{
	this->plugin = plugin;
	this->color_button = 0;
	this->color_picker = 0;
	this->title_x = 0;    this->point_x = 0;
	this->title_y = 0;    this->point_y = 0;
	this->new_point = 0;  this->del_point = 0;
	this->point_up = 0;   this->point_dn = 0;
	this->drag = 0;       this->dragging = 0;
	this->last_x = 0;     this->last_y = 0;
	this->points = 0;     this->cur_point = 0;
}

CriKeyWindow::~CriKeyWindow()
{
	delete color_picker;
	delete points;
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
	add_subwindow(color_button = new CriKeyColorButton(this, x, y));
	int x1 = x + color_button->get_w() + margin;
	color_x = x1;  color_y = y;
	update_color(plugin->config.color);
	y += COLOR_H + 10 + margin ;
	add_subwindow(title = new BC_Title(x, y+5, _("Draw mode:")));
	x1 = x + title->get_w() + 10 + margin;
	add_subwindow(draw_mode = new CriKeyDrawMode(this, x1, y));
	draw_mode->create_objects();
	y += draw_mode->get_h() + 10 + margin;

	CriKeyPoint *pt = plugin->config.points[plugin->config.selected];
	add_subwindow(title_x = new BC_Title(x, y, _("X:")));
	x1 = x + title_x->get_w() + margin;
	point_x = new CriKeyPointX(this, x1, y, pt->x);
	point_x->create_objects();
	x1 += point_x->get_w() + margin;
	add_subwindow(new_point = new CriKeyNewPoint(this, plugin, x1, y));
	x1 += new_point->get_w() + margin;
	add_subwindow(point_up = new CriKeyPointUp(this, x1, y));
	y += point_x->get_h() + margin;
	add_subwindow(title_y = new BC_Title(x, y, _("Y:")));
	x1 = x + title_y->get_w() + margin;
	point_y = new CriKeyPointY(this, x1, y, pt->y);
	point_y->create_objects();
	x1 += point_y->get_w() + margin;
	add_subwindow(del_point = new CriKeyDelPoint(this, plugin, x1, y));
	x1 += del_point->get_w() + margin;
	add_subwindow(point_dn = new CriKeyPointDn(this, x1, y));
	y += point_y->get_h() + margin + 10;

	add_subwindow(drag = new CriKeyDrag(this, x, y));
	if( plugin->config.drag ) {
		if( !grab(plugin->server->mwindow->cwindow->gui) )
			eprintf("drag enabled, but compositor already grabbed\n");
	}

	x1 = x + drag->get_w() + margin + 20;
	add_subwindow(cur_point = new CriKeyCurPoint(this, plugin, x1, y+3));
	cur_point->update(plugin->config.selected);
	y += drag->get_h() + margin;
	add_subwindow(points = new CriKeyPoints(this, plugin, x, y));
	points->update(plugin->config.selected);

	show_window(1);
}

int CriKeyWindow::grab_event(XEvent *event)
{
	switch( event->type ) {
	case ButtonPress: break;
	case ButtonRelease: break;
	case MotionNotify: break;
	default: return 0;
	}

	MWindow *mwindow = plugin->server->mwindow;
	CWindowGUI *cwindow_gui = mwindow->cwindow->gui;
	CWindowCanvas *canvas = cwindow_gui->canvas;
	int cx, cy;  cwindow_gui->get_relative_cursor(cx, cy);
	cx -= mwindow->theme->ccanvas_x;
	cy -= mwindow->theme->ccanvas_y;

	if( !dragging ) {
		if( cx < 0 || cx >= mwindow->theme->ccanvas_w ) return 0;
		if( cy < 0 || cy >= mwindow->theme->ccanvas_h ) return 0;
	}

	switch( event->type ) {
	case ButtonPress:
		if( dragging ) return 0;
		dragging = event->xbutton.state & ShiftMask ? -1 : 1;
		break;
	case ButtonRelease:
		if( !dragging ) return 0;
		dragging = 0;
		return 1;
	case MotionNotify:
		if( !dragging ) return 0;
		break;
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
	point_x->update((int64_t)(output_x));
	point_y->update((int64_t)(output_y));

	if( dragging > 0 ) {
		switch( event->type ) {
		case ButtonPress: {
			int hot_point = -1;
			int n = plugin->config.points.size();
			if( n > 0 ) {
				CriKeyPoint *pt = plugin->config.points[hot_point=0];
				double dist = DISTANCE(output_x,output_y, pt->x,pt->y);
				for( int i=1; i<n; ++i ) {
					pt = plugin->config.points[i];
					double d = DISTANCE(output_x,output_y, pt->x,pt->y);
					if( d >= dist ) continue;
					dist = d;  hot_point = i;
				}
			}
			if( hot_point >= 0 ) {
				CriKeyPoint *pt = plugin->config.points[hot_point];
				if( pt->x == output_x && pt->y == output_y ) break;
				points->set_point(hot_point, PT_X, pt->x = output_x);
				points->set_point(hot_point, PT_Y, pt->y = output_y);
				plugin->config.selected = hot_point;
				points->set_selected(hot_point);
				points->update_list();
			}
			break; }
		case MotionNotify: {
			int hot_point = points->get_selection_number(0, 0);
			if( hot_point >= 0 && hot_point < plugin->config.points.size() ) {
				CriKeyPoint *pt = plugin->config.points[hot_point];
				if( pt->x == output_x && pt->y == output_y ) break;
				points->set_point(hot_point, PT_X, pt->x = output_x);
				points->set_point(hot_point, PT_Y, pt->y = output_y);
				point_x->update(pt->x);
				point_y->update(pt->y);
				points->update_list();
			}
			break; }
		}
	}
	else {
		switch( event->type ) {
		case MotionNotify: {
			float dx = output_x - last_x, dy = output_y - last_y;
			int n = plugin->config.points.size();
			for( int i=0; i<n; ++i ) {
				CriKeyPoint *pt = plugin->config.points[i];
				points->set_point(i, PT_X, pt->x += dx);
				points->set_point(i, PT_Y, pt->y += dy);
			}
			int hot_point = points->get_selection_number(0, 0);
			if( hot_point >= 0 && hot_point < n ) {
				CriKeyPoint *pt = plugin->config.points[hot_point];
				point_x->update(pt->x);
				point_y->update(pt->y);
				points->update_list();
			}
			break; }
		}
	}

	last_x = output_x;  last_y = output_y;
	plugin->send_configure_change();
	return 1;
}

void CriKeyWindow::done_event(int result)
{
	ungrab(client->server->mwindow->cwindow->gui);
	if( color_picker ) color_picker->close_window();
}

CriKeyPoints::CriKeyPoints(CriKeyWindow *gui, CriKey *plugin, int x, int y)
 : BC_ListBox(x, y, 360, 130, LISTBOX_TEXT)
{
	this->gui = gui;
	this->plugin = plugin;
	titles[PT_E] = _("E");    widths[PT_E] = 40;
	titles[PT_X] = _("X");    widths[PT_X] = 120;
	titles[PT_Y] = _("Y");    widths[PT_Y] = 120;
	titles[PT_T] = _("Tag");  widths[PT_T] = 60;
}
CriKeyPoints::~CriKeyPoints()
{
	clear();
}
void CriKeyPoints::clear()
{
	for( int i=PT_SZ; --i>=0; )
		cols[i].remove_all_objects();
}

int CriKeyPoints::column_resize_event()
{
	for( int i=PT_SZ; --i>=0; )
		widths[i] = get_column_width(i);
	return 1;
}

int CriKeyPoints::handle_event()
{
	int hot_point = get_selection_number(0, 0);
	const char *x_text = "", *y_text = "";
	if( hot_point >= 0 && hot_point < plugin->config.points.size() ) {
		if( get_cursor_x() < widths[0] ) {
			plugin->config.points[hot_point]->e =
				!plugin->config.points[hot_point]->e;
		}
		x_text = gui->points->cols[PT_X].get(hot_point)->get_text();
		y_text = gui->points->cols[PT_Y].get(hot_point)->get_text();
	}
	else
		hot_point = 0;
	gui->point_x->update(x_text);
	gui->point_y->update(y_text);
	plugin->config.selected = hot_point;
	update(hot_point);
	gui->plugin->send_configure_change();
	return 1;
}

int CriKeyPoints::selection_changed()
{
	handle_event();
	return 1;
}

void CriKeyPoints::new_point(const char *ep, const char *xp, const char *yp, const char *tp)
{
	cols[PT_E].append(new BC_ListBoxItem(ep));
	cols[PT_X].append(new BC_ListBoxItem(xp));
	cols[PT_Y].append(new BC_ListBoxItem(yp));
	cols[PT_T].append(new BC_ListBoxItem(tp));
}

void CriKeyPoints::del_point(int i)
{
	for( int n=cols[0].size()-1, c=PT_SZ; --c>=0; )
		cols[c].remove_object_number(n-i);
}

void CriKeyPoints::set_point(int i, int c, float v)
{
	char s[BCSTRLEN]; sprintf(s,"%0.4f",v);
	set_point(i,c,s);
}
void CriKeyPoints::set_point(int i, int c, const char *cp)
{
	cols[c].get(i)->set_text(cp);
}

int CriKeyPoints::set_selected(int k)
{
	int sz = gui->plugin->config.points.size();
	if( !sz ) return -1;
	bclamp(k, 0, sz-1);
	int n = gui->points->get_selection_number(0, 0);
	if( n >= 0 ) {
		for( int i=0; i<PT_SZ; ++i )
			cols[i].get(n)->set_selected(0);
	}
	for( int i=0; i<PT_SZ; ++i )
		cols[i].get(k)->set_selected(1);
	gui->cur_point->update(k);
	return k;
}
void CriKeyPoints::update_list()
{
	int xpos = get_xposition(), ypos = get_xposition();
	int k =  get_selection_number(0, 0);
	BC_ListBox::update(&cols[0], &titles[0],&widths[0],PT_SZ, xpos,ypos,k);
}
void CriKeyPoints::update(int k)
{
	if( k < 0 ) k = get_selection_number(0, 0);
	gui->cur_point->update(k);
	int xpos = get_xposition(), ypos = get_xposition();

	clear();
	ArrayList<CriKeyPoint*> &points = plugin->config.points;
	int n = points.size();
	for( int i=0; i<n; ++i ) {
		CriKeyPoint *pt = points[i];
		char etxt[BCSTRLEN];  sprintf(etxt,"%s", pt->e ? "*" : "");
		char xtxt[BCSTRLEN];  sprintf(xtxt,"%0.4f", pt->x);
		char ytxt[BCSTRLEN];  sprintf(ytxt,"%0.4f", pt->y);
		char ttxt[BCSTRLEN];  sprintf(ttxt,"%d", pt->t);
		new_point(etxt, xtxt, ytxt, ttxt);
	}
	if( k < n ) {
		for( int i=PT_SZ; --i>=0; )
			cols[i].get(k)->set_selected(1);
		gui->point_x->update(gui->points->cols[PT_X].get(k)->get_text());
		gui->point_y->update(gui->points->cols[PT_Y].get(k)->get_text());
	}

	BC_ListBox::update(&cols[0], &titles[0],&widths[0],PT_SZ, xpos,ypos,k);
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
	draw_mode->update(plugin->config.draw_mode);
	update_color(plugin->config.color);
	threshold->update(plugin->config.threshold);
	cur_point->update(plugin->config.selected);
	drag->update(plugin->config.drag);
	points->update(plugin->config.selected);
}


CriKeyThreshold::CriKeyThreshold(CriKeyWindow *gui, int x, int y, int w)
 : BC_FSlider(x, y, 0, w, w, 0, 1, gui->plugin->config.threshold, 0)
{
	this->gui = gui;
	set_precision(0.005);
}

int CriKeyThreshold::handle_event()
{
	float v = get_value();
	gui->plugin->config.threshold = v;
	gui->plugin->send_configure_change();
	return 1;
}


CriKeyPointUp::CriKeyPointUp(CriKeyWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Up"))
{
	this->gui = gui;
}
CriKeyPointUp::~CriKeyPointUp()
{
}

int CriKeyPointUp::handle_event()
{
	int n = gui->plugin->config.points.size();
	int hot_point = gui->points->get_selection_number(0, 0);

	if( n > 1 && hot_point > 0 ) {
		CriKeyPoint *&pt0 = gui->plugin->config.points[hot_point];
		CriKeyPoint *&pt1 = gui->plugin->config.points[--hot_point];
		CriKeyPoint *t = pt0;  pt0 = pt1;  pt1 = t;
		gui->plugin->config.selected = hot_point;
		gui->points->update(hot_point);
	}
	gui->plugin->send_configure_change();
	return 1;
}

CriKeyPointDn::CriKeyPointDn(CriKeyWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Dn"))
{
	this->gui = gui;
}
CriKeyPointDn::~CriKeyPointDn()
{
}

int CriKeyPointDn::handle_event()
{
	int n = gui->plugin->config.points.size();
	int hot_point = gui->points->get_selection_number(0, 0);
	if( n > 1 && hot_point < n-1 ) {
		CriKeyPoint *&pt0 = gui->plugin->config.points[hot_point];
		CriKeyPoint *&pt1 = gui->plugin->config.points[++hot_point];
		CriKeyPoint *t = pt0;  pt0 = pt1;  pt1 = t;
		gui->plugin->config.selected = hot_point;
		gui->points->update(hot_point);
	}
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
	CWindowGUI *cwindow_gui = gui->plugin->server->mwindow->cwindow->gui;
	int value = get_value();
	if( value ) {
		if( !gui->grab(cwindow_gui) ) {
			update(value = 0);
			flicker(10,50);
		}
	}
	else
		gui->ungrab(cwindow_gui);
	gui->plugin->config.drag = value;
	gui->plugin->send_configure_change();
	return 1;
}

CriKeyNewPoint::CriKeyNewPoint(CriKeyWindow *gui, CriKey *plugin, int x, int y)
 : BC_GenericButton(x, y, 80, _("New"))
{
	this->gui = gui;
	this->plugin = plugin;
}
CriKeyNewPoint::~CriKeyNewPoint()
{
}
int CriKeyNewPoint::handle_event()
{
	int k = plugin->new_point();
	gui->points->update(k);
	gui->plugin->send_configure_change();
	return 1;
}

CriKeyDelPoint::CriKeyDelPoint(CriKeyWindow *gui, CriKey *plugin, int x, int y)
 : BC_GenericButton(x, y, 80, C_("Del"))
{
	this->gui = gui;
	this->plugin = plugin;
}
CriKeyDelPoint::~CriKeyDelPoint()
{
}
int CriKeyDelPoint::handle_event()
{
	int hot_point = gui->points->get_selection_number(0, 0);
	if( hot_point >= 0 && hot_point < gui->plugin->config.points.size() ) {
		plugin->config.del_point(hot_point);
		if( !plugin->config.points.size() ) plugin->new_point();
		int n = gui->plugin->config.points.size();
		if( hot_point >= n && hot_point > 0 ) --hot_point;
		gui->plugin->config.selected = hot_point;
		gui->points->update(hot_point);
		gui->plugin->send_configure_change();
	}
	return 1;
}

CriKeyCurPoint::CriKeyCurPoint(CriKeyWindow *gui, CriKey *plugin, int x, int y)
 : BC_Title(x, y, "")
{
	this->gui = gui;
	this->plugin = plugin;
}
CriKeyCurPoint::~CriKeyCurPoint()
{
}
void CriKeyCurPoint::update(int n)
{
	char string[BCSTRLEN];
	sprintf(string, _("Selected: %d   "), n);
	BC_Title::update(string);
}

