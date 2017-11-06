
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

#include "autoconf.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindowgui.h"
#include "language.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "tracks.h"
#include "trackcanvas.h"
#include "zoombar.h"

#include <math.h>



GWindowGUI::GWindowGUI(MWindow *mwindow, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Overlays"),
	mwindow->session->gwindow_x, mwindow->session->gwindow_y,
	w, h, w, h, 0, 0, 1)
{
	this->mwindow = mwindow;
	color_thread = 0;
}

GWindowGUI::~GWindowGUI()
{
	delete color_thread;
}

void GWindowGUI::start_color_thread(GWindowColorButton *color_button)
{
	unlock_window();
	delete color_thread;
	color_thread = new GWindowColorThread(color_button);
	int color = auto_colors[color_button->auto_toggle->info->ref];
	color_thread->start(color);
	lock_window("GWindowGUI::start_color_thread");
}

const char *GWindowGUI::other_text[NON_AUTOMATION_TOTAL] =
{
	N_("Assets"),
	N_("Titles"),
	N_("Transitions"),
	N_("Plugin Autos")
};

const char *GWindowGUI::auto_text[AUTOMATION_TOTAL] =
{
	N_("Mute"),
	N_("Camera X"),
	N_("Camera Y"),
	N_("Camera Z"),
	N_("Projector X"),
	N_("Projector Y"),
	N_("Projector Z"),
	N_("Fade"),
	N_("Pan"),
	N_("Mode"),
	N_("Mask"),
	N_("Speed")
};

int GWindowGUI::auto_colors[AUTOMATION_TOTAL] =
{
	PINK,
	RED,
	GREEN,
	BLUE,
	LTPINK,
	LTGREEN,
	LTBLUE,
	LTPURPLE,
	0,
	0,
	0,
	ORANGE,
};

void GWindowGUI::load_defaults()
{
	BC_Hash *defaults = mwindow->defaults;
	auto_colors[AUTOMATION_MUTE] = defaults->get("AUTO_COLOR_MUTE", auto_colors[AUTOMATION_MUTE]);
	auto_colors[AUTOMATION_CAMERA_X] = defaults->get("AUTO_COLOR_CAMERA_X", auto_colors[AUTOMATION_CAMERA_X]);
	auto_colors[AUTOMATION_CAMERA_Y] = defaults->get("AUTO_COLOR_CAMERA_Y", auto_colors[AUTOMATION_CAMERA_Y]);
	auto_colors[AUTOMATION_CAMERA_Z] = defaults->get("AUTO_COLOR_CAMERA_Z", auto_colors[AUTOMATION_CAMERA_Z]);
	auto_colors[AUTOMATION_PROJECTOR_X] = defaults->get("AUTO_COLOR_PROJECTOR_X", auto_colors[AUTOMATION_PROJECTOR_X]);
	auto_colors[AUTOMATION_PROJECTOR_Y] = defaults->get("AUTO_COLOR_PROJECTOR_Y", auto_colors[AUTOMATION_PROJECTOR_Y]);
	auto_colors[AUTOMATION_PROJECTOR_Z] = defaults->get("AUTO_COLOR_PROJECTOR_Z", auto_colors[AUTOMATION_PROJECTOR_Z]);
	auto_colors[AUTOMATION_FADE] = defaults->get("AUTO_COLOR_FADE", auto_colors[AUTOMATION_FADE]);
	auto_colors[AUTOMATION_SPEED] = defaults->get("AUTO_COLOR_SPEED", auto_colors[AUTOMATION_SPEED]);
}

void GWindowGUI::save_defaults()
{
	BC_Hash *defaults = mwindow->defaults;
	defaults->update("AUTO_COLOR_MUTE", auto_colors[AUTOMATION_MUTE]);
	defaults->update("AUTO_COLOR_CAMERA_X", auto_colors[AUTOMATION_CAMERA_X]);
	defaults->update("AUTO_COLOR_CAMERA_Y", auto_colors[AUTOMATION_CAMERA_Y]);
	defaults->update("AUTO_COLOR_CAMERA_Z", auto_colors[AUTOMATION_CAMERA_Z]);
	defaults->update("AUTO_COLOR_PROJECTOR_X", auto_colors[AUTOMATION_PROJECTOR_X]);
	defaults->update("AUTO_COLOR_PROJECTOR_Y", auto_colors[AUTOMATION_PROJECTOR_Y]);
	defaults->update("AUTO_COLOR_PROJECTOR_Z", auto_colors[AUTOMATION_PROJECTOR_Z]);
	defaults->update("AUTO_COLOR_FADE", auto_colors[AUTOMATION_FADE]);
	defaults->update("AUTO_COLOR_SPEED", auto_colors[AUTOMATION_SPEED]);
}

static toggleinfo toggle_order[] =
{
	{0, NON_AUTOMATION_ASSETS},
	{0, NON_AUTOMATION_TITLES},
	{0, NON_AUTOMATION_TRANSITIONS},
	{0, NON_AUTOMATION_PLUGIN_AUTOS},
	{0, -1}, // bar
	{1, AUTOMATION_FADE},
	{1, AUTOMATION_MUTE},
	{1, AUTOMATION_SPEED},
	{1, AUTOMATION_CAMERA_X},
	{1, AUTOMATION_CAMERA_Y},
	{1, AUTOMATION_CAMERA_Z},
	{1, AUTOMATION_PROJECTOR_X},
	{1, AUTOMATION_PROJECTOR_Y},
	{1, AUTOMATION_PROJECTOR_Z},
	{0, -1}, // bar
	{1, AUTOMATION_MODE},
	{1, AUTOMATION_PAN},
	{1, AUTOMATION_MASK},
};

void GWindowGUI::calculate_extents(BC_WindowBase *gui, int *w, int *h)
{
	int temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	int current_w, current_h;
	*w = 10;
	*h = 10;

	for( int i=0; i<(int)(sizeof(toggle_order)/sizeof(toggle_order[0])); ++i ) {
		toggleinfo *tp = &toggle_order[i];
		int isauto = tp->isauto, ref = tp->ref;
		if( ref < 0 ) {
			*h += get_resources()->bar_data->get_h() + 5;
			continue;
		}
		BC_Toggle::calculate_extents(gui,
			BC_WindowBase::get_resources()->checkbox_images,
			0, &temp1, &current_w, &current_h,
			&temp2, &temp3, &temp4, &temp5, &temp6, &temp7,
			_(isauto ?  auto_text[ref] : other_text[ref]),
			MEDIUMFONT);
		current_w += current_h;
		*w = MAX(current_w, *w);
		*h += current_h + 5;
	}

	*h += 10;
	*w += 20;
}

GWindowColorButton::GWindowColorButton(GWindowToggle *auto_toggle, int x, int y, int w)
 : BC_Button(x, y, w, vframes)
{
	this->auto_toggle = auto_toggle;
	this->color = 0;
	for( int i=0; i<3; ++i ) {
		vframes[i] = new VFrame(w, w, BC_RGBA8888, -1);
		vframes[i]->clear_frame();
	}
}

GWindowColorButton::~GWindowColorButton()
{
	for( int i=0; i<3; ++i )
		delete vframes[i];
}

void GWindowColorButton::set_color(int color)
{
	this->color = color;
	int r = (color>>16) & 0xff;
	int g = (color>>8) & 0xff;
	int b = (color>>0) & 0xff;
	for( int i=0; i<3; ++i ) {
		VFrame *vframe = vframes[i];
		int ww = vframe->get_w(), hh = vframe->get_h();
		int cx = (ww+1)/2, cy = hh/2;
		double cc = (cx*cx + cy*cy) / 4.;
		uint8_t *bp = vframe->get_data(), *dp = bp;
		uint8_t *ep = dp + vframe->get_data_size();
		int rr = r, gg = g, bb = b;
		int bpl = vframe->get_bytes_per_line();
		switch( i ) {
		case BUTTON_UP:
			break;
		case BUTTON_UPHI:
			if( (rr+=48) > 0xff ) rr = 0xff;
			if( (gg+=48) > 0xff ) gg = 0xff;
			if( (bb+=48) > 0xff ) bb = 0xff;
			break;
		case BUTTON_DOWNHI:
			if( (rr-=48) < 0x00 ) rr = 0x00;
			if( (gg-=48) < 0x00 ) gg = 0x00;
			if( (bb-=48) < 0x00 ) bb = 0x00;
			break;
		}
		while( dp < ep ) {
			int yy = (dp-bp) / bpl, xx = ((dp-bp) % bpl) >> 2;
			int dy = cy - yy, dx = cx - xx;
			double s = dx*dx + dy*dy - cc;
			double ss = s < 0 ? 1 : s >= cc ? 0 : 1 - s/cc;
			int aa = ss * 0xff;
			*dp++ = rr; *dp++ = gg; *dp++ = bb; *dp++ = aa;
		}
	}
	set_images(vframes);
}

void GWindowColorButton::update_gui(int color)
{
	set_color(color);
	draw_face();
}

GWindowColorThread::GWindowColorThread(GWindowColorButton *color_button)
 : ColorPicker(0, color_button->auto_toggle->caption)
{
	this->color = 0;
	this->color_button = color_button;
	color_update = new GWindowColorUpdate(this);
}

GWindowColorThread::~GWindowColorThread()
{
	delete color_update;
}

void GWindowColorThread::start(int color)
{
	start_window(color, 0, 1);
	color_update->start();
}

void GWindowColorThread::handle_done_event(int result)
{
	color_update->stop();
	GWindowGUI *gui = color_button->auto_toggle->gui;
	int ref = color_button->auto_toggle->info->ref;
	gui->lock_window("GWindowColorThread::handle_done_event");
	if( !result ) {
		GWindowGUI::auto_colors[ref] = color;
		color_button->auto_toggle->update_gui(color);
		gui->save_defaults();
	}
	else {
		color = GWindowGUI::auto_colors[ref];
		color_button->update_gui(color);
	}
	gui->unlock_window();
	MWindowGUI *mwindow_gui = color_button->auto_toggle->gui->mwindow->gui;
	mwindow_gui->lock_window("GWindowColorUpdate::run");
	mwindow_gui->draw_overlays(1);
	mwindow_gui->unlock_window();
}

int GWindowColorThread::handle_new_color(int color, int alpha)
{
	this->color = color;
	color_update->update_lock->unlock();
	return 1;
}

void GWindowColorThread::update_gui()
{
	color_button->update_gui(color);
}

GWindowColorUpdate::GWindowColorUpdate(GWindowColorThread *color_thread)
 : Thread(1, 0, 0)
{
	this->color_thread = color_thread;
	this->update_lock = new Condition(0,"GWindowColorUpdate::update_lock");
	done = 1;
}

GWindowColorUpdate::~GWindowColorUpdate()
{
	stop();
	delete update_lock;
}

void GWindowColorUpdate::start()
{
	if( done ) {
		done = 0;
		Thread::start();
	}
}

void GWindowColorUpdate::stop()
{
	if( !done ) {
		done = 1;
		update_lock->unlock();
		join();
	}
}

void GWindowColorUpdate::run()
{
	while( !done ) {
		update_lock->lock("GWindowColorUpdate::run");
		if( done ) break;
		color_thread->update_gui();
	}
}


int GWindowColorButton::handle_event()
{
	GWindowGUI *gui = auto_toggle->gui;
	gui->start_color_thread(this);
	return 1;
}

void GWindowGUI::create_objects()
{
	int x = 10, y = 10;
	lock_window("GWindowGUI::create_objects 1");

	for( int i=0; i<(int)(sizeof(toggle_order)/sizeof(toggle_order[0])); ++i ) {
		toggleinfo *tp = &toggle_order[i];
		int ref = tp->ref;
		if( ref < 0 ) {
			BC_Bar *bar = new BC_Bar(x,y,get_w()-x-10);
			add_tool(bar);
			toggles[i] = 0;
			y += bar->get_h() + 5;
			continue;
		}
		VFrame *vframe = 0;
		switch( ref ) {
		case AUTOMATION_MODE: vframe = mwindow->theme->modekeyframe_data;  break;
		case AUTOMATION_PAN:  vframe = mwindow->theme->pankeyframe_data;   break;
		case AUTOMATION_MASK: vframe = mwindow->theme->maskkeyframe_data;  break;
		}
		const char *label = _(tp->isauto ? auto_text[tp->ref] : other_text[tp->ref]);
		int color = !tp->isauto ? -1 : auto_colors[tp->ref];
		GWindowToggle *toggle = new GWindowToggle(mwindow, this, x, y, label, color, tp);
		add_tool(toggles[i] = toggle);
		if( vframe )
			draw_vframe(vframe, get_w()-vframe->get_w()-10, y);
		else if( tp->isauto ) {
			int wh = toggle->get_h() - 4;
			GWindowColorButton *color_button =
				new GWindowColorButton(toggle, get_w()-wh-10, y+2, wh);
			add_tool(color_button);
			color_button->set_color(color);
			color_button->draw_face();
		}
		y += toggles[i]->get_h() + 5;
	}
	unlock_window();
}

void GWindowGUI::update_mwindow()
{
	unlock_window();
	mwindow->gui->mainmenu->update_toggles(1);
	lock_window("GWindowGUI::update_mwindow");
}

void GWindowGUI::update_toggles(int use_lock)
{
	if(use_lock) lock_window("GWindowGUI::update_toggles");

	for( int i=0; i<(int)(sizeof(toggle_order)/sizeof(toggle_order[0])); ++i ) {
		if( toggles[i] ) toggles[i]->update();
	}

	if(use_lock) unlock_window();
}

int GWindowGUI::translation_event()
{
	mwindow->session->gwindow_x = get_x();
	mwindow->session->gwindow_y = get_y();
	return 0;
}

int GWindowGUI::close_event()
{
	delete color_thread;  color_thread = 0;
	hide_window();
	mwindow->session->show_gwindow = 0;
	unlock_window();

	mwindow->gui->lock_window("GWindowGUI::close_event");
	mwindow->gui->mainmenu->show_gwindow->set_checked(0);
	mwindow->gui->unlock_window();

	lock_window("GWindowGUI::close_event");
	mwindow->save_defaults();
	return 1;
}

int GWindowGUI::keypress_event()
{
	switch(get_keypress()) {
	case 'w':
	case 'W':
	case '0':
		if( ctrl_down() ) {
			close_event();
			return 1;
		}
		break;
	}
	return 0;
}


GWindowToggle::GWindowToggle(MWindow *mwindow, GWindowGUI *gui, int x, int y,
	const char *text, int color, toggleinfo *info)
 : BC_CheckBox(x, y, *get_main_value(mwindow, info), text, MEDIUMFONT, color)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->info = info;
	this->color = color;
	this->color_button = 0;
}

GWindowToggle::~GWindowToggle()
{
	delete color_button;
}

int GWindowToggle::handle_event()
{
	int value = get_value();
	*get_main_value(mwindow, info) = value;
	gui->update_mwindow();


// Update stuff in MWindow
	unlock_window();
	mwindow->gui->lock_window("GWindowToggle::handle_event");
	if( info->isauto ) {
		int autogroup_type = -1;
		switch( info->ref ) {
		case AUTOMATION_FADE:
			autogroup_type = mwindow->edl->tracks->recordable_video_tracks() ?
				AUTOGROUPTYPE_VIDEO_FADE : AUTOGROUPTYPE_AUDIO_FADE ;
			break;
		case AUTOMATION_SPEED:
			autogroup_type = AUTOGROUPTYPE_SPEED;
			break;
		case AUTOMATION_CAMERA_X:
		case AUTOMATION_PROJECTOR_X:
			autogroup_type = AUTOGROUPTYPE_X;
			break;
		case AUTOMATION_CAMERA_Y:
		case AUTOMATION_PROJECTOR_Y:
			autogroup_type = AUTOGROUPTYPE_Y;
			break;
		case AUTOMATION_CAMERA_Z:
		case AUTOMATION_PROJECTOR_Z:
			autogroup_type = AUTOGROUPTYPE_ZOOM;
			break;
		}
		if( value && autogroup_type >= 0 ) {
			mwindow->edl->local_session->zoombar_showautotype = autogroup_type;
			mwindow->gui->zoombar->update_autozoom();
		}
		mwindow->gui->draw_overlays(1);
	}
	else {
		switch( info->ref ) {
		case NON_AUTOMATION_ASSETS:
		case NON_AUTOMATION_TITLES:
			mwindow->gui->update(1, 1, 0, 0, 1, 0, 0);
			break;

		case NON_AUTOMATION_TRANSITIONS:
		case NON_AUTOMATION_PLUGIN_AUTOS:
			mwindow->gui->draw_overlays(1);
			break;
		}
	}

	mwindow->gui->unlock_window();
	lock_window("GWindowToggle::handle_event");

	return 1;
}

int* GWindowToggle::get_main_value(MWindow *mwindow, toggleinfo *info)
{
	if( info->isauto )
		return &mwindow->edl->session->auto_conf->autos[info->ref];

	switch( info->ref ) {
	case NON_AUTOMATION_ASSETS: return &mwindow->edl->session->show_assets;
	case NON_AUTOMATION_TITLES: return &mwindow->edl->session->show_titles;
	case NON_AUTOMATION_TRANSITIONS: return &mwindow->edl->session->auto_conf->transitions;
	case NON_AUTOMATION_PLUGIN_AUTOS: return &mwindow->edl->session->auto_conf->plugins;
	}
	return 0;
}

void GWindowToggle::update()
{
	int *vp = get_main_value(mwindow, info);
	if( !vp ) return;
	set_value(*vp);
}

void GWindowToggle::update_gui(int color)
{
	BC_Toggle::color = color;
	draw_face(1,0);
}

