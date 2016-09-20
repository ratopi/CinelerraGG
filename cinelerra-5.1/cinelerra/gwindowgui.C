
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
#include "bcsignals.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindowgui.h"
#include "language.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "trackcanvas.h"





GWindowGUI::GWindowGUI(MWindow *mwindow, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Overlays"),
	mwindow->session->gwindow_x, mwindow->session->gwindow_y,
	w, h, w, h, 0, 0, 1)
{
	this->mwindow = mwindow;
}

const char *GWindowGUI::other_text[NON_AUTOMATION_TOTAL] =
{
	"Assets",
	"Titles",
	"Transitions",
	"Plugin Autos"
};

const char *GWindowGUI::auto_text[AUTOMATION_TOTAL] =
{
	"Mute",
	"Camera X",
	"Camera Y",
	"Camera Z",
	"Projector X",
	"Projector Y",
	"Projector Z",
	"Fade",
	"Pan",
	"Mode",
	"Mask",
	"Speed"
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
		*w = MAX(current_w, *w);
		*h += current_h + 5;
	}

	*h += 10;
	*w += 20;
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
		toggles[i] = new GWindowToggle(mwindow, this, x, y, *tp);
		add_tool(toggles[i]);
		VFrame *vframe = 0;
		switch( ref ) {
		case AUTOMATION_MODE: vframe = mwindow->theme->modekeyframe_data;  break;
		case AUTOMATION_PAN:  vframe = mwindow->theme->pankeyframe_data;   break;
		case AUTOMATION_MASK: vframe = mwindow->theme->maskkeyframe_data;  break;
		}
		if( vframe )
			draw_vframe(vframe, get_w()-vframe->get_w()-10, y);
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


GWindowToggle::GWindowToggle(MWindow *mwindow,
	GWindowGUI *gui, int x, int y, toggleinfo toggleinf)
 : BC_CheckBox(x, y, *get_main_value(mwindow, toggleinf),
	_((toggleinf.isauto ?
		GWindowGUI::auto_text[toggleinf.ref] :
		GWindowGUI::other_text[toggleinf.ref])),
	MEDIUMFONT,
	!toggleinf.isauto ? -1 : GWindowGUI::auto_colors[toggleinf.ref])
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->toggleinf = toggleinf;
}

int GWindowToggle::handle_event()
{
	*get_main_value(mwindow, toggleinf) = get_value();
	gui->update_mwindow();


// Update stuff in MWindow
	unlock_window();
	mwindow->gui->lock_window("GWindowToggle::handle_event");
	if(toggleinf.isauto)
	{
		mwindow->gui->draw_overlays(1);
	}
	else
	{
		switch(toggleinf.ref)
		{
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

int* GWindowToggle::get_main_value(MWindow *mwindow, toggleinfo toggleinf)
{
	if( toggleinf.isauto )
		return &mwindow->edl->session->auto_conf->autos[toggleinf.ref];

	switch(toggleinf.ref) {
	case NON_AUTOMATION_ASSETS: return &mwindow->edl->session->show_assets;
	case NON_AUTOMATION_TITLES: return &mwindow->edl->session->show_titles;
	case NON_AUTOMATION_TRANSITIONS: return &mwindow->edl->session->auto_conf->transitions;
	case NON_AUTOMATION_PLUGIN_AUTOS: return &mwindow->edl->session->auto_conf->plugins;
	}
	return 0;
}

void GWindowToggle::update()
{
	int *vp = get_main_value(mwindow, toggleinf);
	if( !vp ) return;
	set_value(*vp);
}



