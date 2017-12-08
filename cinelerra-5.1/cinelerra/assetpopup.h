
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

#ifndef ASSETPOPUP_H
#define ASSETPOPUP_H

#include "assetedit.inc"
#include "assetpopup.inc"
#include "awindowgui.inc"
#include "edl.inc"
#include "guicast.h"
#include "assets.inc"
#include "mwindow.inc"



class AssetPopup : public BC_PopupMenu
{
public:
	AssetPopup(MWindow *mwindow, AWindowGUI *gui);
	~AssetPopup();

	void create_objects();
// Set mainsession with the current selections
	int update();
	void paste_assets();
	void match_size();
	void match_rate();
	void match_all();

	MWindow *mwindow;
	AWindowGUI *gui;


	AssetPopupInfo *info;
	AssetPopupBuildIndex *index;
	AssetPopupView *view;
	AssetPopupViewWindow *view_window;
	AssetPopupMixer *mixer;
	AWindowListFormat *format;
};

class AssetPopupInfo : public BC_MenuItem
{
public:
	AssetPopupInfo(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupInfo();

	int handle_event();
	int button_press_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetPopupSortNames : public BC_MenuItem
{
public:
	AssetPopupSortNames(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupSortNames();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetPopupSortTimes : public BC_MenuItem
{
public:
	AssetPopupSortTimes(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupSortTimes();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetPopupBuildIndex : public BC_MenuItem
{
public:
	AssetPopupBuildIndex(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupBuildIndex();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};


class AssetPopupView : public BC_MenuItem
{
public:
	AssetPopupView(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupView();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};


class AssetPopupViewWindow : public BC_MenuItem
{
public:
	AssetPopupViewWindow(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupViewWindow();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetPopupMixer : public BC_MenuItem
{
public:
	AssetPopupMixer(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupMixer();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetPopupPaste : public BC_MenuItem
{
public:
	AssetPopupPaste(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupPaste();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetMatchSize : public BC_MenuItem
{
public:
	AssetMatchSize(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetMatchRate : public BC_MenuItem
{
public:
	AssetMatchRate(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetMatchAll : public BC_MenuItem
{
public:
	AssetMatchAll(MWindow *mwindow, AssetPopup *popup);

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetPopupProjectRemove : public BC_MenuItem
{
public:
	AssetPopupProjectRemove(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupProjectRemove();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetPopupDiskRemove : public BC_MenuItem
{
public:
	AssetPopupDiskRemove(MWindow *mwindow, AssetPopup *popup);
	~AssetPopupDiskRemove();

	int handle_event();

	MWindow *mwindow;
	AssetPopup *popup;
};

class AssetListMenu : public BC_PopupMenu
{
public:
	AssetListMenu(MWindow *mwindow, AWindowGUI *gui);
	~AssetListMenu();

	void create_objects();
	void update_titles();

	MWindow *mwindow;
	AWindowGUI *gui;
	AWindowListFormat *format;
};

class AssetListCopy : public BC_MenuItem
{
public:
	AssetListCopy(MWindow *mwindow, AWindowGUI *gui);
	~AssetListCopy();

	int handle_event();

	MWindow *mwindow;
	AWindowGUI *gui;
	AssetCopyDialog *copy_dialog;
};

class AssetCopyDialog : public BC_DialogThread
{
public:
	AssetCopyDialog(AssetListCopy *copy);
	~AssetCopyDialog();

	void start(char *text, int x, int y);
	BC_Window* new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);

	char *text;
	int x, y;
	AssetListCopy *copy;
	AssetCopyWindow *copy_window;
};

class AssetCopyWindow : public BC_Window
{
public:
	AssetCopyWindow(AssetCopyDialog *copy_dialog);
	~AssetCopyWindow();

	void create_objects();
	int resize_event(int w, int h);

	AssetCopyDialog *copy_dialog;
	BC_ScrollTextBox *file_list;
};

class AssetListPaste : public BC_MenuItem
{
public:
	AssetListPaste(MWindow *mwindow, AWindowGUI *gui);
	~AssetListPaste();

	int handle_event();

	MWindow *mwindow;
	AWindowGUI *gui;
	AssetPasteDialog *paste_dialog;
};

class AssetPasteDialog : public BC_DialogThread
{
public:
	AssetPasteDialog(AssetListPaste *paste);
	~AssetPasteDialog();

	BC_Window* new_gui();
	void handle_done_event(int result);
	void handle_close_event(int result);
	void start(int x, int y);

	AssetListPaste *paste;
	AssetPasteWindow *paste_window;
	int x, y;
};

class AssetPasteWindow : public BC_Window
{
public:
	AssetPasteWindow(AssetPasteDialog *paste_dialog);
	~AssetPasteWindow();

	void create_objects();
	int resize_event(int w, int h);

	AssetPasteDialog *paste_dialog;
	BC_ScrollTextBox *file_list;
};

#endif
