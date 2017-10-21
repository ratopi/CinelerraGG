
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

#include "asset.h"
#include "assets.h"
#include "awindow.h"
#include "awindowgui.h"
#include "edit.h"
#include "editpopup.h"
#include "cache.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "preferences.h"
#include "renderengine.h"
#include "resizetrackthread.h"
#include "track.h"
#include "tracks.h"
#include "trackcanvas.h"
#include "transportque.h"
#include "vframe.h"
#include "vrender.h"

#include <string.h>

EditPopup::EditPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

EditPopup::~EditPopup()
{
}

void EditPopup::create_objects()
{
	add_item(new EditAttachEffect(mwindow, this));
	add_item(new EditMoveTrackUp(mwindow, this));
	add_item(new EditMoveTrackDown(mwindow, this));
	add_item(new EditPopupDeleteTrack(mwindow, this));
	add_item(new EditPopupAddTrack(mwindow, this));
//	add_item(new EditPopupTitle(mwindow, this));
	EditSnapshot *edit_snapshot;
	SnapshotSubMenu *snapshot_submenu;
	add_item(edit_snapshot = new EditSnapshot(mwindow, this));
	edit_snapshot->add_submenu(snapshot_submenu = new SnapshotSubMenu(edit_snapshot));
	snapshot_submenu->add_submenuitem(new SnapshotMenuItem(snapshot_submenu, _("png"),  SNAPSHOT_PNG));
	snapshot_submenu->add_submenuitem(new SnapshotMenuItem(snapshot_submenu, _("jpeg"), SNAPSHOT_JPEG));
	snapshot_submenu->add_submenuitem(new SnapshotMenuItem(snapshot_submenu, _("tiff"), SNAPSHOT_TIFF));
	resize_option = 0;
	matchsize_option = 0;
}

int EditPopup::update(Track *track, Edit *edit)
{
	this->edit = edit;
	this->track = track;

	if(track->data_type == TRACK_VIDEO && !resize_option)
	{
		add_item(resize_option = new EditPopupResize(mwindow, this));
		add_item(matchsize_option = new EditPopupMatchSize(mwindow, this));
	}
	else
	if(track->data_type == TRACK_AUDIO && resize_option)
	{
		del_item(resize_option);     resize_option = 0;
		del_item(matchsize_option);  matchsize_option = 0;
	}
	return 0;
}









EditAttachEffect::EditAttachEffect(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Attach effect..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new PluginDialogThread(mwindow);
}

EditAttachEffect::~EditAttachEffect()
{
	delete dialog_thread;
}

int EditAttachEffect::handle_event()
{
	dialog_thread->start_window(popup->track,
		0, _(PROGRAM_NAME ": Attach Effect"),
		0, popup->track->data_type);
	return 1;
}


EditMoveTrackUp::EditMoveTrackUp(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditMoveTrackUp::~EditMoveTrackUp()
{
}
int EditMoveTrackUp::handle_event()
{
	mwindow->move_track_up(popup->track);
	return 1;
}



EditMoveTrackDown::EditMoveTrackDown(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Move down"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditMoveTrackDown::~EditMoveTrackDown()
{
}
int EditMoveTrackDown::handle_event()
{
	mwindow->move_track_down(popup->track);
	return 1;
}


EditPopupResize::EditPopupResize(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Resize track..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new ResizeTrackThread(mwindow);
}
EditPopupResize::~EditPopupResize()
{
	delete dialog_thread;
}

int EditPopupResize::handle_event()
{
	dialog_thread->start_window(popup->track);
	return 1;
}


EditPopupMatchSize::EditPopupMatchSize(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Match output size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
EditPopupMatchSize::~EditPopupMatchSize()
{
}

int EditPopupMatchSize::handle_event()
{
	mwindow->match_output_size(popup->track);
	return 1;
}


EditPopupDeleteTrack::EditPopupDeleteTrack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int EditPopupDeleteTrack::handle_event()
{
	mwindow->delete_track(popup->track);
	return 1;
}


EditPopupAddTrack::EditPopupAddTrack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Add track"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditPopupAddTrack::handle_event()
{
	switch( popup->track->data_type ) {
	case TRACK_AUDIO:
		mwindow->add_audio_track_entry(1, popup->track);
		break;
	case TRACK_VIDEO:
		mwindow->add_video_track_entry(popup->track);
		break;
	case TRACK_SUBTITLE:
		mwindow->add_subttl_track_entry(popup->track);
		break;
	}
	return 1;
}


EditPopupTitle::EditPopupTitle(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("User title..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	window = 0;
}

EditPopupTitle::~EditPopupTitle()
{
	delete popup;
}

int EditPopupTitle::handle_event()
{
	int result;

	Track *trc = mwindow->session->track_highlighted;

	if (trc && trc->record)
	{
		Edit *edt = mwindow->session->edit_highlighted;
		if(!edt) return 1;

		window = new EditPopupTitleWindow (mwindow, popup);
		window->create_objects();
		result = window->run_window();


		if(!result && edt)
		{
			strcpy(edt->user_title, window->title_text->get_text());
		}

		delete window;
		window = 0;
	}

	return 1;
}


EditPopupTitleWindow::EditPopupTitleWindow (MWindow *mwindow, EditPopup *popup)
 : BC_Window (_(PROGRAM_NAME ": Set edit title"),
	mwindow->gui->get_abs_cursor_x(0) - 400 / 2,
	mwindow->gui->get_abs_cursor_y(0) - 500 / 2,
	300,
	100,
	300,
	100,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->popup = popup;
	this->edt = this->mwindow->session->edit_highlighted;
	if(this->edt)
	{
		strcpy(new_text, this->edt->user_title);
	}
}

EditPopupTitleWindow::~EditPopupTitleWindow()
{
}

int EditPopupTitleWindow::close_event()
{
	set_done(1);
	return 1;
}

void EditPopupTitleWindow::create_objects()
{
	int x = 5;
	int y = 10;

	add_subwindow (new BC_Title (x, y, _("User title")));
	add_subwindow (title_text = new EditPopupTitleText (this,
		mwindow, x, y + 20));
	add_tool(new BC_OKButton(this));
	add_tool(new BC_CancelButton(this));


	show_window();
	flush();
}


EditPopupTitleText::EditPopupTitleText (EditPopupTitleWindow *window,
	MWindow *mwindow, int x, int y)
 : BC_TextBox(x, y, 250, 1, (char*)(window->edt ? window->edt->user_title : ""))
{
	this->window = window;
	this->mwindow = mwindow;
}

EditPopupTitleText::~EditPopupTitleText()
{
}

int EditPopupTitleText::handle_event()
{
	return 1;
}



EditSnapshot::EditSnapshot(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Snapshot..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

EditSnapshot::~EditSnapshot()
{
}

SnapshotSubMenu::SnapshotSubMenu(EditSnapshot *edit_snapshot)
{
	this->edit_snapshot = edit_snapshot;
}

SnapshotSubMenu::~SnapshotSubMenu()
{
}

SnapshotMenuItem::SnapshotMenuItem(SnapshotSubMenu *submenu, const char *text, int mode)
 : BC_MenuItem(text)
{
	this->submenu = submenu;
	this->mode = mode;
}

SnapshotMenuItem::~SnapshotMenuItem()
{
}

int SnapshotMenuItem::handle_event()
{
	MWindow *mwindow = submenu->edit_snapshot->mwindow;
	EDL *edl = mwindow->edl;
	if( !edl->have_video() ) return 1;

	Preferences *preferences = mwindow->preferences;
	char filename[BCTEXTLEN];
	static const char *exts[] = { "png", "jpg", "tif" };
	time_t tt;     time(&tt);
	struct tm tm;  localtime_r(&tt,&tm);
	sprintf(filename,"%s/snap_%04d%02d%02d-%02d%02d%02d.%s",
		preferences->snapshot_path,
		1900+tm.tm_year,1+tm.tm_mon,tm.tm_mday,
		tm.tm_hour,tm.tm_min,tm.tm_sec, exts[mode]);
	int fw = edl->get_w(), fh = edl->get_h();
	int fcolor_model = edl->session->color_model;

	Asset *asset = new Asset(filename);
	switch( mode ) {
	case SNAPSHOT_PNG:
		asset->format = FILE_PNG;
		asset->png_use_alpha = 1;
		break;
	case SNAPSHOT_JPEG:
		asset->format = FILE_JPEG;
		asset->jpeg_quality = 90;
		break;
	case SNAPSHOT_TIFF:
		asset->format = FILE_TIFF;
		asset->tiff_cmodel = 0;
		asset->tiff_compression = 0;
		break;
	}
	asset->width = fw;
	asset->height = fh;
	asset->audio_data = 0;
	asset->video_data = 1;
	asset->video_length = 1;
	asset->layers = 1;

	File file;
	int processors = preferences->project_smp + 1;
	if( processors > 8 ) processors = 8;
	file.set_processors(processors);
	int ret = file.open_file(preferences, asset, 0, 1);
	if( !ret ) {
		file.start_video_thread(1, fcolor_model,
			processors > 1 ? 2 : 1, 0);
		VFrame ***frames = file.get_video_buffer();
		VFrame *frame = frames[0][0];
		TransportCommand command;
		//command.command = audio_tracks ? NORMAL_FWD : CURRENT_FRAME;
		command.command = CURRENT_FRAME;
		command.get_edl()->copy_all(edl);
		command.change_type = CHANGE_ALL;
		command.realtime = 0;

		RenderEngine render_engine(0, preferences, 0, 0);
		CICache video_cache(preferences);
		render_engine.set_vcache(&video_cache);
		render_engine.arm_command(&command);

		double position = edl->local_session->get_selectionstart(1);
		int64_t source_position = (int64_t)(position * edl->get_frame_rate());
		int ret = render_engine.vrender->process_buffer(frame, source_position, 0);
		if( !ret )
			ret = file.write_video_buffer(1);
		file.close_file();
	}
	if( !ret ) {
		asset->awindow_folder = AW_MEDIA_FOLDER;
		mwindow->edl->assets->append(asset);
		mwindow->awindow->gui->async_update_assets();
	}
	else {
		eprintf("snapshot render failed");
		asset->remove_user();
	}
	return 1;
}

