
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

#ifndef KEYFRAMEPOPUP_H
#define KEYFRAMEPOPUP_H

#include "guicast.h"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "plugin.inc"
#include "plugindialog.inc"
#include "keyframe.inc"
#include "keyframepopup.inc"
#include "automation.h" 
#include "floatauto.h"


 
class KeyframePopup : public BC_PopupMenu
{
public:
	KeyframePopup(MWindow *mwindow, MWindowGUI *gui);
	~KeyframePopup();

	void create_objects();
	int update(Plugin *plugin, KeyFrame *keyframe);
	int update(Automation *automation, Autos *autos, Auto *auto_keyframe);

	MWindow *mwindow;
	MWindowGUI *gui;
// Acquired through the update command as the plugin currently being operated on
	
	Plugin *keyframe_plugin;
	Autos *keyframe_autos;
	Automation *keyframe_automation;
	Auto *keyframe_auto;
	BC_MenuItem *key_mbar;

private:	
	KeyframePopupDelete *key_delete;
	KeyframePopupShow *key_show;
	KeyframePopupCopy *key_copy;
	KeyframePopupEdit *key_edit;
	KeyframePopupCurveMode *key_smooth;
	KeyframePopupCurveMode *key_linear;
	KeyframePopupCurveMode *key_free_t;
	KeyframePopupCurveMode *key_free;
	bool key_edit_displayed;
	bool key_mode_displayed;

	void handle_curve_mode(Autos *autos, Auto *auto_keyframe);
};

class KeyframePopupDelete : public BC_MenuItem
{
public:
	KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup);
	~KeyframePopupDelete();
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupShow : public BC_MenuItem
{
public:
	KeyframePopupShow(MWindow *mwindow, KeyframePopup *popup);
	~KeyframePopupShow();
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupCopy : public BC_MenuItem
{
public:
	KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup);
	~KeyframePopupCopy();
	int handle_event();
	
	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframePopupCurveMode : public BC_MenuItem
{
public:
	KeyframePopupCurveMode(MWindow *mwindow, KeyframePopup *popup, int curve_mode);
	~KeyframePopupCurveMode();
	int handle_event();

private:
	MWindow *mwindow;
	KeyframePopup *popup;
	int curve_mode;
	const char* get_labeltext(int);
	void toggle_mode(FloatAuto*);
    
friend class KeyframePopup;
};

 
class KeyframePopupEdit : public BC_MenuItem
{
public:
	KeyframePopupEdit(MWindow *mwindow, KeyframePopup *popup);
	int handle_event();

	MWindow *mwindow;
	KeyframePopup *popup;
};

class KeyframeHidePopup : public BC_PopupMenu
{
public:
        KeyframeHidePopup(MWindow *mwindow, MWindowGUI *gui);
        ~KeyframeHidePopup();

        void create_objects();
	int update(Autos *autos);

	MWindow *mwindow;
	MWindowGUI *gui;
	Autos *keyframe_autos;
};

class KeyframePopupHide : public BC_MenuItem
{
public:
	KeyframePopupHide(MWindow *mwindow, KeyframeHidePopup *popup);
	int handle_event();

	MWindow *mwindow;
	KeyframeHidePopup *popup;
};

#endif
