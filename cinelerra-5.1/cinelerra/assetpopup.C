
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

#include "asset.h"
#include "assetedit.h"
#include "assetpopup.h"
#include "assetremove.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "cstrdup.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "tracks.h"
#include "vwindow.h"
#include "vwindowgui.h"


AssetPopup::AssetPopup(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetPopup::~AssetPopup()
{
}

void AssetPopup::create_objects()
{
	BC_MenuItem *menu_item;
	BC_SubMenu *submenu;
	add_item(info = new AssetPopupInfo(mwindow, this));
	add_item(format = new AWindowListFormat(mwindow));
	add_item(new AssetPopupSort(mwindow, this));
	add_item(index = new AssetPopupBuildIndex(mwindow, this));
	add_item(view = new AssetPopupView(mwindow, this));
	add_item(view_window = new AssetPopupViewWindow(mwindow, this));
	add_item(new AssetPopupPaste(mwindow, this));
	add_item(menu_item = new BC_MenuItem(_("Match...")));
	menu_item->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new AssetMatchSize(mwindow, this));
	submenu->add_submenuitem(new AssetMatchRate(mwindow, this));
	submenu->add_submenuitem(new AssetMatchAll(mwindow, this));
	add_item(menu_item = new BC_MenuItem(_("Remove...")));
	menu_item->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new AssetPopupProjectRemove(mwindow, this));
	submenu->add_submenuitem(new AssetPopupDiskRemove(mwindow, this));
}

void AssetPopup::paste_assets()
{
// Collect items into the drag vectors for temporary storage
	gui->lock_window("AssetPopup::paste_assets");
	mwindow->gui->lock_window("AssetPopup::paste_assets");
	mwindow->cwindow->gui->lock_window("AssetPopup::paste_assets");

	gui->collect_assets();
	mwindow->paste_assets(mwindow->edl->local_session->get_selectionstart(1),
		mwindow->edl->tracks->first,
		0);   // do not overwrite

	gui->unlock_window();
	mwindow->gui->unlock_window();
	mwindow->cwindow->gui->unlock_window();
}

void AssetPopup::match_size()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("AssetPopup::match_size");
	mwindow->asset_to_size();
	mwindow->gui->unlock_window();
}

void AssetPopup::match_rate()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("AssetPopup::match_rate");
	mwindow->asset_to_rate();
	mwindow->gui->unlock_window();
}

void AssetPopup::match_all()
{
// Collect items into the drag vectors for temporary storage
	gui->collect_assets();
	mwindow->gui->lock_window("AssetPopup::match_rate");
	mwindow->asset_to_all();
	mwindow->gui->unlock_window();
}

int AssetPopup::update()
{
	format->update();
	gui->collect_assets();
	return 0;
}


AssetPopupInfo::AssetPopupInfo(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Info..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupInfo::~AssetPopupInfo()
{
}

int AssetPopupInfo::handle_event()
{
	if(mwindow->session->drag_assets->total)
	{
		mwindow->awindow->asset_edit->edit_asset(
			mwindow->session->drag_assets->values[0]);
	}
	else
	if(mwindow->session->drag_clips->total)
	{
		popup->gui->awindow->clip_edit->edit_clip(
			mwindow->session->drag_clips->values[0]);
	}
	return 1;
}


AssetPopupBuildIndex::AssetPopupBuildIndex(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Rebuild index"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupBuildIndex::~AssetPopupBuildIndex()
{
}

int AssetPopupBuildIndex::handle_event()
{
//printf("AssetPopupBuildIndex::handle_event 1\n");
	mwindow->rebuild_indices();
	return 1;
}


AssetPopupSort::AssetPopupSort(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Sort items"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupSort::~AssetPopupSort()
{
}

int AssetPopupSort::handle_event()
{
	mwindow->awindow->gui->sort_assets();
	return 1;
}


AssetPopupView::AssetPopupView(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("View"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupView::~AssetPopupView()
{
}

int AssetPopupView::handle_event()
{
	VWindow *vwindow = mwindow->get_viewer(1, DEFAULT_VWINDOW);
	vwindow->gui->lock_window("AssetPopupView::handle_event");

	if(mwindow->session->drag_assets->total)
		vwindow->change_source(
			mwindow->session->drag_assets->values[0]);
	else
	if(mwindow->session->drag_clips->total)
		vwindow->change_source(
			mwindow->session->drag_clips->values[0]);

	vwindow->gui->unlock_window();
	return 1;
}


AssetPopupViewWindow::AssetPopupViewWindow(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("View in new window"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupViewWindow::~AssetPopupViewWindow()
{
}

int AssetPopupViewWindow::handle_event()
{
// Find window with nothing
	VWindow *vwindow = mwindow->get_viewer(1);

// TODO: create new vwindow or change current vwindow
	vwindow->gui->lock_window("AssetPopupView::handle_event");

	if(mwindow->session->drag_assets->total)
		vwindow->change_source(
			mwindow->session->drag_assets->values[0]);
	else
	if(mwindow->session->drag_clips->total)
		vwindow->change_source(
			mwindow->session->drag_clips->values[0]);

	vwindow->gui->unlock_window();
	return 1;
}


AssetPopupPaste::AssetPopupPaste(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Paste"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupPaste::~AssetPopupPaste()
{
}

int AssetPopupPaste::handle_event()
{
	popup->paste_assets();
	return 1;
}


AssetMatchSize::AssetMatchSize(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Match project size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int AssetMatchSize::handle_event()
{
	popup->match_size();
	return 1;
}

AssetMatchRate::AssetMatchRate(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Match frame rate"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int AssetMatchRate::handle_event()
{
	popup->match_rate();
	return 1;
}

AssetMatchAll::AssetMatchAll(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Match all"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int AssetMatchAll::handle_event()
{
	popup->match_all();
	return 1;
}


AssetPopupProjectRemove::AssetPopupProjectRemove(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Remove from project"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

AssetPopupProjectRemove::~AssetPopupProjectRemove()
{
}

int AssetPopupProjectRemove::handle_event()
{
	mwindow->remove_assets_from_project(1,
		1,
		mwindow->session->drag_assets,
		mwindow->session->drag_clips);
	return 1;
}


AssetPopupDiskRemove::AssetPopupDiskRemove(MWindow *mwindow, AssetPopup *popup)
 : BC_MenuItem(_("Remove from disk"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}


AssetPopupDiskRemove::~AssetPopupDiskRemove()
{
}

int AssetPopupDiskRemove::handle_event()
{
	mwindow->awindow->asset_remove->start();
	return 1;
}


AssetListMenu::AssetListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

AssetListMenu::~AssetListMenu()
{
}

void AssetListMenu::create_objects()
{
	add_item(format = new AWindowListFormat(mwindow));
	add_item(new AWindowListSort(mwindow));
	add_item(new AssetListCopy(mwindow));
	add_item(new AssetListPaste(mwindow));
	update_titles();
}

void AssetListMenu::update_titles()
{
	format->update();
}

AssetListCopy::AssetListCopy(MWindow *mwindow)
 : BC_MenuItem(_("Copy file list"))
{
	this->mwindow = mwindow;
	copy_dialog = 0;
}
AssetListCopy::~AssetListCopy()
{
	delete copy_dialog;
}

int AssetListCopy::handle_event()
{
	int len = 0;
	MWindowGUI *gui = mwindow->gui;
	gui->lock_window("AssetListCopy::handle_event");
	mwindow->awindow->gui->collect_assets();
	int n = mwindow->session->drag_assets->total;
	for( int i=0; i<n; ++i ) {
		Indexable *indexable = mwindow->session->drag_assets->values[i];
		const char *path = indexable->path;
		if( !*path ) continue;
		len += strlen(path) + 1;
	}
	char *text = new char[len+1], *cp = text;
	for( int i=0; i<n; ++i ) {
		Indexable *indexable = mwindow->session->drag_assets->values[i];
		const char *path = indexable->path;
		if( !*path ) continue;
		cp += sprintf(cp, "%s\n", path);
	}
	*cp = 0;
	gui->unlock_window(); 

	if( n ) {
		if( !copy_dialog )
			copy_dialog = new AssetCopyDialog(this);
		copy_dialog->start(text);
	}
	else {
		eprintf(_("Nothing selected"));
		delete [] text;
	}
	return 1;
}

AssetCopyDialog::AssetCopyDialog(AssetListCopy *copy)
 : BC_DialogThread()
{
        this->copy = copy;
	copy_window = 0;
}

void AssetCopyDialog::start(char *text)
{
        close_window();
        this->text = text;
	BC_DialogThread::start();
}

AssetCopyDialog::~AssetCopyDialog()
{
        close_window();
}

BC_Window* AssetCopyDialog::new_gui()
{
        BC_DisplayInfo display_info;
        int x = display_info.get_abs_cursor_x();
        int y = display_info.get_abs_cursor_y();

        copy_window = new AssetCopyWindow(this, x, y);
        copy_window->create_objects();
        return copy_window;
}

void AssetCopyDialog::handle_done_event(int result)
{
	delete [] text;  text = 0;
}

void AssetCopyDialog::handle_close_event(int result)
{
        copy_window = 0;
}


AssetCopyWindow::AssetCopyWindow(AssetCopyDialog *copy_dialog, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Copy File List"), x, y, 500, 200, 500, 200, 0, 0, 1)
{
        this->copy_dialog = copy_dialog;
}

AssetCopyWindow::~AssetCopyWindow()
{
}

void AssetCopyWindow::create_objects()
{
	BC_Title *title;
	int x = 10, y = 10, pad = 5;
	add_subwindow(title = new BC_Title(x, y, _("List of asset paths:")));
	y += title->get_h() + pad;
	int text_w = get_w() - x - 10;
	int text_h = get_h() - y - BC_OKButton::calculate_h() - pad;
	int text_rows = BC_TextBox::pixels_to_rows(this, MEDIUMFONT, text_h);
	char *text = copy_dialog->text;
	int len = strlen(text) + BCTEXTLEN;
	file_list = new BC_ScrollTextBox(this, x, y, text_w, text_rows, text, len);
	file_list->create_objects();

	add_subwindow(new BC_OKButton(this));
        show_window();
}


AssetListPaste::AssetListPaste(MWindow *mwindow)
 : BC_MenuItem(_("Paste file list"))
{
	this->mwindow = mwindow;
	paste_dialog = 0;
}
AssetListPaste::~AssetListPaste()
{
	delete paste_dialog;
}

int AssetListPaste::handle_event()
{
	if( !paste_dialog )
		paste_dialog = new AssetPasteDialog(this);
	paste_dialog->start();
	return 1;
}

AssetPasteDialog::AssetPasteDialog(AssetListPaste *paste)
 : BC_DialogThread()
{
        this->paste = paste;
	paste_window = 0;
}

AssetPasteDialog::~AssetPasteDialog()
{
        close_window();
}

BC_Window* AssetPasteDialog::new_gui()
{
        BC_DisplayInfo display_info;
        int x = display_info.get_abs_cursor_x();
        int y = display_info.get_abs_cursor_y();

        paste_window = new AssetPasteWindow(this, x, y);
        paste_window->create_objects();
        return paste_window;
}

void AssetPasteDialog::handle_done_event(int result)
{
	if( result ) return;
	const char *bp = paste_window->file_list->get_text(), *ep = bp+strlen(bp);
	ArrayList<char*> path_list;
	path_list.set_array_delete();

	for( const char *cp=bp; cp<ep && *cp; ) {
		const char *dp = strchr(cp, '\n');
		if( !dp ) dp = ep;
		char path[BCTEXTLEN], *pp = path;
		int len = sizeof(path)-1;
		while( --len>0 && cp<dp ) *pp++ = *cp++;
		if( *cp ) ++cp;
		*pp = 0;
		if( !strlen(path) ) continue;
		path_list.append(cstrdup(path));
	}
	if( !path_list.size() ) return;

	MWindow *mwindow = paste->mwindow;
	mwindow->interrupt_indexes();
	mwindow->gui->lock_window("AssetPasteDialog::handle_done_event");
	result = mwindow->load_filenames(&path_list, LOADMODE_RESOURCESONLY, 0);
	mwindow->gui->unlock_window();
	path_list.remove_all_objects();
        mwindow->save_backup();
        mwindow->restart_brender();
	mwindow->session->changes_made = 1;
}

void AssetPasteDialog::handle_close_event(int result)
{
        paste_window = 0;
}


AssetPasteWindow::AssetPasteWindow(AssetPasteDialog *paste_dialog, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Paste File List"), x, y, 500, 200, 500, 200, 0, 0, 1)
{
        this->paste_dialog = paste_dialog;
}

AssetPasteWindow::~AssetPasteWindow()
{
}

void AssetPasteWindow::create_objects()
{
	BC_Title *title;
	int x = 10, y = 10, pad = 5;
	add_subwindow(title = new BC_Title(x, y, _("Enter list of asset paths:")));
	y += title->get_h() + pad;
	int text_w = get_w() - x - 10;
	int text_h = get_h() - y - BC_OKButton::calculate_h() - pad;
	int text_rows = BC_TextBox::pixels_to_rows(this, MEDIUMFONT, text_h);
	file_list = new BC_ScrollTextBox(this, x, y, text_w, text_rows, (char*)0, 65536);
	file_list->create_objects();
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
        show_window();
}

