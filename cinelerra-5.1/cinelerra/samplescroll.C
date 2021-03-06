
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

#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "samplescroll.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"

SampleScroll::SampleScroll(MWindow *mwindow,
	MWindowGUI *gui,
	int x,
	int y,
	int w)
 : BC_ScrollBar(x,
 	y,
	SCROLL_HORIZ,
	w,
	0,
	0,
	0)
{
	this->gui = gui;
	this->pane = 0;
	this->mwindow = mwindow;
	oldposition = 0;
}

SampleScroll::SampleScroll(MWindow *mwindow,
	TimelinePane *pane,
	int x,
	int y,
	int w)
 : BC_ScrollBar(x,
 	y,
	SCROLL_HORIZ,
	w,
	0,
	0,
	0)
{
	this->gui = mwindow->gui;
	this->pane = pane;
	this->mwindow = mwindow;
	oldposition = 0;
}

SampleScroll::~SampleScroll()
{
}

int SampleScroll::flip_vertical()
{
	return 0;
}

int SampleScroll::in_use()
{
	return in_use();
}


int SampleScroll::resize_event()
{
	reposition_window(mwindow->theme->mhscroll_x,
		mwindow->theme->mhscroll_y,
		mwindow->theme->mhscroll_w);
	return 0;
}

int SampleScroll::resize_event(int x, int y, int w)
{
	reposition_window(x,
		y,
		w);
	return 0;
}

int SampleScroll::set_position()
{
	if(!pane->canvas) return 0;
	long length = Units::round(mwindow->edl->tracks->total_length() *
		mwindow->edl->session->sample_rate /
		mwindow->edl->local_session->zoom_sample);
	long position = mwindow->edl->local_session->view_start[pane->number];
	long handle_size = pane->view_w;

	update_length(length,
			position,
			handle_size,
			0);

	oldposition = position;
	return 0;
}

int SampleScroll::handle_event()
{
//printf("SampleScroll::handle_event %d %d\n", __LINE__, get_window_lock());
	mwindow->edl->local_session->view_start[pane->number] = get_value();
	if(pane->number == TOP_LEFT_PANE ||
		pane->number == BOTTOM_LEFT_PANE)
	{
		mwindow->edl->local_session->view_start[TOP_LEFT_PANE] =
			mwindow->edl->local_session->view_start[BOTTOM_LEFT_PANE] =
			mwindow->edl->local_session->view_start[pane->number];
	}
	else
	{
		mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] =
			mwindow->edl->local_session->view_start[BOTTOM_RIGHT_PANE] =
			mwindow->edl->local_session->view_start[pane->number];
	}

	mwindow->gui->draw_canvas(0, 1);
	mwindow->gui->draw_cursor(1);
	mwindow->gui->update_timebar(0);
	mwindow->gui->flash_canvas(0);

//	if(pane->patchbay) pane->patchbay->update();


//printf("SampleScroll::handle_event %d %d\n", __LINE__, get_window_lock());

	return 1;
}
