
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#include "assetedit.h"
#include "clippopup.h"
#include "assetremove.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "filexml.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "tracks.h"
#include "vwindow.h"
#include "vwindowgui.h"



ClipPopup::ClipPopup(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

ClipPopup::~ClipPopup()
{
}

void ClipPopup::create_objects()
{
	BC_MenuItem *menu_item;
	BC_SubMenu *submenu;
	add_item(info = new ClipPopupInfo(mwindow, this));
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(new ClipPopupSort(mwindow, this));
	add_item(view = new ClipPopupView(mwindow, this));
	add_item(view_window = new ClipPopupViewWindow(mwindow, this));
	add_item(new ClipPopupCopy(mwindow, this));
	add_item(new ClipPopupPaste(mwindow, this));
	add_item(menu_item = new BC_MenuItem(_("Match...")));
	menu_item->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new ClipMatchSize(mwindow, this));
	submenu->add_submenuitem(new ClipMatchRate(mwindow, this));
	submenu->add_submenuitem(new ClipMatchAll(mwindow, this));
	add_item(new ClipPopupDelete(mwindow, this));
}

void ClipPopup::paste_assets()
{
// Collect items into the drag vectors for temporary storage
	gui->lock_window("ClipPopup::paste_assets");
	mwindow->gui->lock_window("ClipPopup::paste_assets");
	mwindow->cwindow->gui->lock_window("ClipPopup::paste_assets");

	gui->collect_assets();
	mwindow->paste_assets(mwindow->edl->local_session->get_selectionstart(1),
		mwindow->edl->tracks->first,
		0);   // do not overwrite

	gui->unlock_window();
	mwindow->gui->unlock_window();
	mwindow->cwindow->gui->unlock_window();
}

void ClipPopup::match_size()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("ClipPopup::match_size");
	mwindow->asset_to_size();
	mwindow->gui->unlock_window();
}

void ClipPopup::match_rate()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("ClipPopup::match_rate");
	mwindow->asset_to_rate();
	mwindow->gui->unlock_window();
}

void ClipPopup::match_all()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("ClipPopup::match_rate");
	mwindow->asset_to_all();
	mwindow->gui->unlock_window();
}

int ClipPopup::update()
{
	format->update();
	gui->collect_assets();
	return 0;
}


ClipPopupInfo::ClipPopupInfo(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("Info..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ClipPopupInfo::~ClipPopupInfo()
{
}

int ClipPopupInfo::handle_event()
{
	int cur_x, cur_y;
	popup->gui->get_abs_cursor_xy(cur_x, cur_y, 0);

	if( mwindow->session->drag_assets->total ) {
		mwindow->awindow->asset_edit->edit_asset(
			mwindow->session->drag_assets->values[0], cur_x, cur_y);
	}
	else
	if( mwindow->session->drag_clips->total ) {
		popup->gui->awindow->clip_edit->edit_clip(
			mwindow->session->drag_clips->values[0], cur_x, cur_y);
	}
	return 1;
}


ClipPopupSort::ClipPopupSort(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("Sort items"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ClipPopupSort::~ClipPopupSort()
{
}

int ClipPopupSort::handle_event()
{
	mwindow->awindow->gui->sort_assets();
	return 1;
}


ClipPopupView::ClipPopupView(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("View"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ClipPopupView::~ClipPopupView()
{
}

int ClipPopupView::handle_event()
{
	VWindow *vwindow = mwindow->get_viewer(1, DEFAULT_VWINDOW);
	vwindow->gui->lock_window("ClipPopupView::handle_event");

	if( mwindow->session->drag_assets->total )
		vwindow->change_source(
			mwindow->session->drag_assets->values[0]);
	else
	if( mwindow->session->drag_clips->total )
		vwindow->change_source(
			mwindow->session->drag_clips->values[0]);

	vwindow->gui->unlock_window();
	return 1;
}


ClipPopupViewWindow::ClipPopupViewWindow(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("View in new window"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ClipPopupViewWindow::~ClipPopupViewWindow()
{
}

int ClipPopupViewWindow::handle_event()
{
// Find window with nothing
	VWindow *vwindow = mwindow->get_viewer(1);

// TODO: create new vwindow or change current vwindow
	vwindow->gui->lock_window("ClipPopupView::handle_event");

	if( mwindow->session->drag_assets->total )
		vwindow->change_source(
			mwindow->session->drag_assets->values[0]);
	else
	if( mwindow->session->drag_clips->total )
		vwindow->change_source(
			mwindow->session->drag_clips->values[0]);

	vwindow->gui->unlock_window();
	return 1;
}


ClipPopupCopy::ClipPopupCopy(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("Copy"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
ClipPopupCopy::~ClipPopupCopy()
{
}

int ClipPopupCopy::handle_event()
{
	MWindowGUI *gui = mwindow->gui;
	gui->lock_window("ClipPopupCopy::handle_event");
	if( mwindow->session->drag_clips->total > 0 ) {
		FileXML file;
		EDL *edl = mwindow->session->drag_clips->values[0];
		double start = 0, end = edl->tracks->total_length();
		edl->copy(start, end, 1, 0, 0, &file, "", 1);
		const char *file_string = file.string();
		long file_length = strlen(file_string);
		gui->get_clipboard()->to_clipboard(file_string, file_length,
			SECONDARY_SELECTION);
		gui->get_clipboard()->to_clipboard(file_string, file_length,
			BC_PRIMARY_SELECTION);
	}
	gui->unlock_window(); 
	return 1;
}


ClipPopupPaste::ClipPopupPaste(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("Paste"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ClipPopupPaste::~ClipPopupPaste()
{
}

int ClipPopupPaste::handle_event()
{
	popup->paste_assets();
	return 1;
}


ClipMatchSize::ClipMatchSize(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("Match project size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int ClipMatchSize::handle_event()
{
	popup->match_size();
	return 1;
}


ClipMatchRate::ClipMatchRate(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("Match frame rate"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int ClipMatchRate::handle_event()
{
	popup->match_rate();
	return 1;
}


ClipMatchAll::ClipMatchAll(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("Match all"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int ClipMatchAll::handle_event()
{
	popup->match_all();
	return 1;
}


ClipPopupDelete::ClipPopupDelete(MWindow *mwindow, ClipPopup *popup)
 : BC_MenuItem(_("Delete"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}


ClipPopupDelete::~ClipPopupDelete()
{
}

int ClipPopupDelete::handle_event()
{
	mwindow->remove_assets_from_project(1, 1,
		mwindow->session->drag_assets,
		mwindow->session->drag_clips);
	return 1;
}


ClipPasteToFolder::ClipPasteToFolder(MWindow *mwindow)
 : BC_MenuItem(_("Paste Clip"))
{
	this->mwindow = mwindow;
}

int ClipPasteToFolder::handle_event()
{
	MWindowGUI *gui = mwindow->gui;
	gui->lock_window("ClipPasteToFolder::handle_event 1");
	int64_t len = gui->get_clipboard()->clipboard_len(SECONDARY_SELECTION);
	if( len ) {
		char *string = new char[len + 1];
		gui->get_clipboard()->from_clipboard(string, len, BC_PRIMARY_SELECTION);
		const char *clip_header = "<EDL VERSION=";
		if( !strncmp(clip_header, string, strlen(clip_header)) ) {
			FileXML file;
			file.read_from_string(string);
			EDL *edl = mwindow->edl;
			EDL *new_edl = new EDL(mwindow->edl);
			new_edl->create_objects();
			new_edl->load_xml(&file, LOAD_ALL);
			edl->update_assets(new_edl);
			mwindow->save_clip(new_edl, _("paste clip: "));
		}
		else {
			char *cp = strchr(string, '\n');
			if( cp-string < 32 ) *cp = 0;
			else if( len > 32 ) string[32] = 0;
			eprintf("paste buffer is not EDL:\n%s", string);
		}
		delete [] string;
	}
	else {
		eprintf("paste buffer empty");
	}
	gui->unlock_window();
	return 1;
}


ClipListMenu::ClipListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

ClipListMenu::~ClipListMenu()
{
}

void ClipListMenu::create_objects()
{
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(new AWindowListSort(mwindow, gui));
	add_item(new ClipPasteToFolder(mwindow));
	update();
}

void ClipListMenu::update()
{
	format->update();
}

