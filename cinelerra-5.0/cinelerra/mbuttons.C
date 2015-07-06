
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

#include "bcsignals.h"
#include "cplayback.h"
#include "cwindow.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keys.h"
#include "localsession.h"
#include "mbuttons.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "record.h"
#include "mainsession.h"
#include "theme.h"
#include "tracks.h"

MButtons::MButtons(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mbuttons_x, 
 	mwindow->theme->mbuttons_y, 
	mwindow->theme->mbuttons_w, 
	mwindow->theme->mbuttons_h)
{
	this->gui = gui;
	this->mwindow = mwindow; 
}

MButtons::~MButtons()
{
	delete transport;
	delete edit_panel;
}

void MButtons::create_objects()
{
	int x = 3, y = 0;
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	transport = new MainTransport(mwindow, this, x, y);
	transport->create_objects();
	transport->set_engine(mwindow->cwindow->playback_engine);
	x += transport->get_w();
	x += mwindow->theme->mtransport_margin;

	edit_panel = new MainEditing(mwindow, this, x, y);

	edit_panel->create_objects();
	
	x += edit_panel->get_w();
	ffmpeg_toggle = new MainFFMpegToggle(mwindow, this, get_w()-30, 0);
	add_subwindow(ffmpeg_toggle);
	flash(0);
}

int MButtons::resize_event()
{
	reposition_window(mwindow->theme->mbuttons_x, 
 		mwindow->theme->mbuttons_y, 
		mwindow->theme->mbuttons_w, 
		mwindow->theme->mbuttons_h);
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	flash(0);
	return 0;
}

int MButtons::keypress_event()
{
	int result = 0;

	if(!result)
	{
		result = transport->keypress_event();
	}

	return result;
}

void MButtons::update()
{
	edit_panel->update();
}
















MainTransport::MainTransport(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : PlayTransport(mwindow, mbuttons, x, y)
{
}

void MainTransport::goto_start()
{
	mwindow->gui->unlock_window();
	handle_transport(REWIND, 1);
	mwindow->gui->lock_window();
	mwindow->goto_start();
}


void MainTransport::goto_end()
{
	mwindow->gui->unlock_window();
	handle_transport(GOTO_END, 1);
	mwindow->gui->lock_window();
	mwindow->goto_end();
}

MainEditing::MainEditing(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : EditPanel(mwindow, 
		mbuttons, 
		x, 
		y,
		mwindow->edl->session->editing_mode,
		1,
		1,
		0, 
		0, 
		1, 
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		0,
		1,
		1,
		mwindow->has_commercials())
{
	this->mwindow = mwindow;
	this->mbuttons = mbuttons;
}


#include "data/ff_checked_png.h"
#include "data/ff_down_png.h"
#include "data/ff_checkedhi_png.h"
#include "data/ff_up_png.h"
#include "data/ff_hi_png.h"

static VFrame *ff_images[] = {
	new VFrame(ff_up_png),
	new VFrame(ff_hi_png),
	new VFrame(ff_checked_png),
	new VFrame(ff_down_png),
	new VFrame(ff_checkedhi_png)
};

MainFFMpegToggle::MainFFMpegToggle(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : BC_Toggle(x - ff_images[0]->get_w(), y, &ff_images[0],
		mwindow->preferences->ffmpeg_early_probe)
{
	this->mwindow = mwindow;
	this->mbuttons = mbuttons;
	set_tooltip("FFMpeg early probe");
}

MainFFMpegToggle::~MainFFMpegToggle()
{
}

int MainFFMpegToggle::handle_event()
{
	mwindow->preferences->ffmpeg_early_probe = get_value();
	mwindow->show_warning(&mwindow->preferences->warn_indecies,
		"Changing the base codecs may require rebuilding indecies.");
	return 1;
}

