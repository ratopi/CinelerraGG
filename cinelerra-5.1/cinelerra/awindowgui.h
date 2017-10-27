
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

#ifndef AWINDOWGUI_H
#define AWINDOWGUI_H

#include "arraylist.h"
#include "bcdialog.h"
#include "assetpopup.inc"
#include "asset.inc"
#include "assets.inc"
#include "awindow.inc"
#include "awindowgui.inc"
#include "clippopup.inc"
#include "edl.inc"
#include "effectlist.inc"
#include "folderlistmenu.inc"
#include "guicast.h"
#include "labeledit.inc"
#include "labelpopup.inc"
#include "labels.h"
#include "indexable.inc"
#include "mwindow.inc"
#include "newfolder.inc"
#include "pluginserver.inc"
#include "vicon.h"

class AssetPicon : public BC_ListBoxItem
{
public:
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, Indexable *indexable);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, EDL *edl);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, PluginServer *plugin);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, Label *plugin);
	AssetPicon(MWindow *mwindow, AWindowGUI *gui, int folder, int persist);
	virtual ~AssetPicon();

	void create_objects();
	void reset();

	MWindow *mwindow;
	AWindowGUI *gui;
	BC_Pixmap *icon;
	VFrame *icon_vframe;
	int foldernum;
// ID of thing pointed to
	int id;

// Check ID first.  Update these next before dereferencing
// Asset if asset
	Indexable *indexable;
// EDL if clip
	EDL *edl;

	int in_use;


	int persistent;
	PluginServer *plugin;
	Label *label;
	VIcon *vicon;
};

class AssetVIcon : public VIcon {
public:
	AssetPicon *picon;
	VFrame *temp;
	int64_t length;

	VFrame *frame();
	int64_t set_seq_no(int64_t no);
	int get_vx();
	int get_vy();

	AssetVIcon(AssetPicon *picon, int w, int h, double framerate, int64_t length);
	~AssetVIcon();
};

class AWindowRemovePlugin;

class AWindowRemovePluginGUI : public BC_Window {
public:
	AWindow *awindow;
	AWindowRemovePlugin *thread;
	PluginServer *plugin;
	ArrayList<BC_ListBoxItem*> plugin_list;
	BC_Pixmap *icon;
	VFrame *icon_vframe;
	BC_ListBox *list;

	void create_objects();

	AWindowRemovePluginGUI(AWindow *awindow, AWindowRemovePlugin *thread,
		 int x, int y, PluginServer *plugin);
	~AWindowRemovePluginGUI();
};

class AWindowRemovePlugin : public BC_DialogThread {
public:
	AWindow *awindow;
	PluginServer *plugin;
	BC_Window* new_gui();
	void handle_close_event(int result);
	int remove_plugin(PluginServer *plugin, ArrayList<BC_ListBoxItem*> &folder);

	AWindowRemovePlugin(AWindow *awindow, PluginServer *plugin);
	~AWindowRemovePlugin();
};

class AWindowGUI : public BC_Window
{
public:
	AWindowGUI(MWindow *mwindow, AWindow *awindow);
	~AWindowGUI();

	void create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();
	int keypress_event();
	void async_update_assets();     // Sends update asset event
	void update_effects();
	void sort_assets();
	void sort_folders();
	void reposition_objects();
	static int folder_number(const char *name);
// Call back for MWindow entry point
	int drag_motion();
	int drag_stop();
// Collect items into the drag vectors of MainSession
	void collect_assets();
	void create_persistent_folder(ArrayList<BC_ListBoxItem*> *output,
		int do_audio,
		int do_video,
		int is_realtime,
		int is_transition);
	void create_label_folder();
	void copy_picons(ArrayList<BC_ListBoxItem*> *dst,
		ArrayList<BC_ListBoxItem*> *src, int folder);
	void sort_picons(ArrayList<BC_ListBoxItem*> *src);
// Return the selected asset in asset_list
	Indexable* selected_asset();
	PluginServer* selected_plugin();
	AssetPicon* selected_folder();
	bool protected_pixmap(BC_Pixmap *pixmap);
	int save_defaults(BC_Hash *defaults);
	int load_defaults(BC_Hash *defaults);
	void start_vicon_drawing();
	void stop_vicon_drawing();
	void update_picon(Indexable *indexable);

	VFrame *get_picon(const char *name, const char *plugin_icons);
	VFrame *get_picon(const char *name);
	void resource_icon(VFrame *&vfrm, BC_Pixmap *&icon, const char *fn, int idx);
	void theme_icon(VFrame *&vfrm, BC_Pixmap *&icon, const char *fn);
	void plugin_icon(VFrame *&vfrm, BC_Pixmap *&icon, const char *fn, unsigned char *png);

	MWindow *mwindow;
	AWindow *awindow;

	AWindowAssets *asset_list;
	AWindowFolders *folder_list;
	AWindowDivider *divider;
	AWindowSearchText *search_text;

// Store data to speed up responses
// Persistant data for listboxes
// All assets in current EDL
	ArrayList<BC_ListBoxItem*> assets;
	ArrayList<BC_ListBoxItem*> folders;
	ArrayList<BC_ListBoxItem*> aeffects;
	ArrayList<BC_ListBoxItem*> veffects;
	ArrayList<BC_ListBoxItem*> atransitions;
	ArrayList<BC_ListBoxItem*> vtransitions;
	ArrayList<BC_ListBoxItem*> labellist;

// Currently displayed data for listboxes
// Currently displayed assets + comments
	ArrayList<BC_ListBoxItem*> displayed_assets[2];
	const char *asset_titles[ASSET_COLUMNS];
	int displayed_folder;

	BC_Hash *defaults;
// Persistent icons
	BC_Pixmap *aeffect_folder_icon;      VFrame *aeffect_folder_vframe;
	BC_Pixmap *atransition_folder_icon;  VFrame *atransition_folder_vframe;
	BC_Pixmap *clip_folder_icon;         VFrame *clip_folder_vframe;
	BC_Pixmap *label_folder_icon;        VFrame *label_folder_vframe;
	BC_Pixmap *media_folder_icon;        VFrame *media_folder_vframe;
	BC_Pixmap *proxy_folder_icon;        VFrame *proxy_folder_vframe;
	BC_Pixmap *veffect_folder_icon;      VFrame *veffect_folder_vframe;
	BC_Pixmap *vtransition_folder_icon;  VFrame *vtransition_folder_vframe;
	BC_Pixmap *folder_icons[AWINDOW_FOLDERS];

	BC_Pixmap *folder_icon;       VFrame *folder_vframe;
	BC_Pixmap *file_icon;         VFrame *file_vframe;
	BC_Pixmap *audio_icon;        VFrame *audio_vframe;
	BC_Pixmap *video_icon;        VFrame *video_vframe;
	BC_Pixmap *label_icon;        VFrame *label_vframe;
	BC_Pixmap *clip_icon;         VFrame *clip_vframe;
	BC_Pixmap *atransition_icon;  VFrame *atransition_vframe;
	BC_Pixmap *vtransition_icon;  VFrame *vtransition_vframe;
	BC_Pixmap *aeffect_icon;      VFrame *aeffect_vframe;
	BC_Pixmap *veffect_icon;      VFrame *veffect_vframe;
	BC_Pixmap *ladspa_icon;       VFrame *ladspa_vframe;
	BC_Pixmap *ff_aud_icon;       VFrame *ff_aud_vframe;
	BC_Pixmap *ff_vid_icon;       VFrame *ff_vid_vframe;

	NewFolderThread *newfolder_thread;

// Popup menus
	AssetPopup *asset_menu;
	ClipPopup *clip_menu;
	LabelPopup *label_menu;
	EffectListMenu *effectlist_menu;
	AssetListMenu *assetlist_menu;
	ClipListMenu *cliplist_menu;
	LabelListMenu *labellist_menu;
	FolderListMenu *folderlist_menu;
	AddTools *add_tools;
// Temporary for reading picons from files
	VFrame *temp_picon;
	VIconThread *vicon_thread;

	int64_t plugin_visibility;
	AWindowRemovePlugin *remove_plugin;

	AVIconDrawing *avicon_drawing;
	int avicon_w, avicon_h, vicon_drawing;
	int allow_iconlisting;

// Create custom atoms to be used for async messages between windows
	int create_custom_xatoms();
// Function to overload to recieve customly defined atoms
	virtual int recieve_custom_xatoms(xatom_event *event);
	static const char *folder_names[];

private:
	void update_folder_list();
	void update_asset_list();
	void filter_displayed_assets();
	Atom UpdateAssetsXAtom;
	void update_assets();

};

class AWindowAssets : public BC_ListBox
{
public:
	AWindowAssets(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
	~AWindowAssets();

	int handle_event();
	int selection_changed();
	void draw_background();
	int drag_start_event();
	int drag_motion_event();
	int drag_stop_event();
	int button_press_event();
	int column_resize_event();
	int focus_in_event();
	int focus_out_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class AWindowDivider : public BC_SubWindow
{
public:
	AWindowDivider(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
	~AWindowDivider();

	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class AWindowFolders : public BC_ListBox
{
public:
	AWindowFolders(MWindow *mwindow, AWindowGUI *gui, int x, int y, int w, int h);
	~AWindowFolders();

	int selection_changed();
	int button_press_event();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class AWindowSearchTextBox : public BC_TextBox
{
public:
	AWindowSearchTextBox(AWindowSearchText *search_text, int x, int y, int w);
	int handle_event();

	AWindowSearchText *search_text;
};

class AWindowSearchText
{
public:
	AWindowSearchText(MWindow *mwindow, AWindowGUI *gui, int x, int y);

	int handle_event();
	void create_objects();

	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
	BC_Title *text_title;
	BC_TextBox *text_box;
	int get_w();
	int get_h();
	void reposition_window(int x, int y, int w);
	const char *get_text();
};

class AWindowNewFolder : public BC_Button
{
public:
	AWindowNewFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowDeleteFolder : public BC_Button
{
public:
	AWindowDeleteFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowRenameFolder : public BC_Button
{
public:
	AWindowRenameFolder(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowDeleteDisk : public BC_Button
{
public:
	AWindowDeleteDisk(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowDeleteProject : public BC_Button
{
public:
	AWindowDeleteProject(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowInfo : public BC_Button
{
public:
	AWindowInfo(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowRedrawIndex : public BC_Button
{
public:
	AWindowRedrawIndex(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowPaste : public BC_Button
{
public:
	AWindowPaste(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowAppend : public BC_Button
{
public:
	AWindowAppend(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AWindowView : public BC_Button
{
public:
	AWindowView(MWindow *mwindow, AWindowGUI *gui, int x, int y);
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
	int x, y;
};

class AddTools : public BC_PopupMenu
{
public:
	AddTools(MWindow *mwindow, AWindowGUI *gui, int x, int y, const char *title);
	void create_objects();

	MWindow *mwindow;
	AWindowGUI *gui;
};

class AddPluginItem : public BC_MenuItem
{
public:
	AddPluginItem(AddTools *menu, const char *text, int idx);
	int handle_event();

	AddTools *menu;
	int idx;
};

class AVIconDrawing : public BC_Toggle
{
public:
	AWindowGUI *agui;

	int handle_event();
	static void calculate_geometry(AWindowGUI *agui, VFrame **images, int *ww, int *hh);

	AVIconDrawing(AWindowGUI *agui, int x, int y, VFrame **images);
	~AVIconDrawing();
};


class AWindowListFormat : public BC_MenuItem
{
public:
	AWindowListFormat(MWindow *mwindow, AWindowGUI *gui);

	void update();
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
};


class AWindowListSort : public BC_MenuItem
{
public:
	AWindowListSort(MWindow *mwindow, AWindowGUI *gui);

	void update();
	int handle_event();
	MWindow *mwindow;
	AWindowGUI *gui;
};

#endif
