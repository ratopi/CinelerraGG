
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

#include "bchash.h"
#include "bcsignals.h"
#include "edl.h"
#include "keyframe.h"
#include "keyframes.h"
#include "keyframegui.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"


KeyFrameThread::KeyFrameThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	plugin = 0;
	keyframe = 0;
	keyframe_data = new ArrayList<BC_ListBoxItem*>[KEYFRAME_COLUMNS];
	plugin_title[0] = 0;
	window_title[0] = 0;
	column_titles[0] = (char*)_("Parameter");
	column_titles[1] = (char*)_("Value");
	column_width[0] = 0;
	column_width[1] = 0;
}

KeyFrameThread::~KeyFrameThread()
{
	close_window();
	for(int i = 0; i < KEYFRAME_COLUMNS; i++)
		keyframe_data[i].remove_all_objects();
	delete [] keyframe_data;
}


void KeyFrameThread::update_values()
{
// Get the current selection before deleting the tables
	int selection = -1;
	for(int i = 0; i < keyframe_data[0].size(); i++) {
		if(keyframe_data[0].get(i)->get_selected()) {
			selection = i;
			break;
		}
	}

	for(int i = 0; i < KEYFRAME_COLUMNS; i++)
		keyframe_data[i].remove_all_objects();


// Must lock main window to read keyframe
	mwindow->gui->lock_window("KeyFrameThread::update_values");
	if(!plugin || !mwindow->edl->tracks->plugin_exists(plugin)) {
		mwindow->gui->unlock_window();
		return;
	}

	KeyFrame *keyframe = 0;
	if(this->keyframe && plugin->keyframe_exists(this->keyframe)) {
// If user edited a specific keyframe, use it.
		keyframe = this->keyframe;
	}
	else if(plugin->track) {
// Use currently highlighted keyframe
		keyframe = plugin->get_prev_keyframe(
			plugin->track->to_units(
				mwindow->edl->local_session->get_selectionstart(1), 0),
			PLAY_FORWARD);
	}

	if(keyframe) {
		BC_Hash hash;
		char *text = 0, *extra = 0;
		keyframe->get_contents(&hash, &text, &extra);
		
		for(int i = 0; i < hash.size(); i++)
		{
			keyframe_data[0].append(new BC_ListBoxItem(hash.get_key(i)));
			keyframe_data[1].append(new BC_ListBoxItem(hash.get_value(i)));
		}
		keyframe_data[0].append(new BC_ListBoxItem((char*)_("TEXT")));
		keyframe_data[1].append(new BC_ListBoxItem(text));
		
		delete [] text;
		delete [] extra;
	}

	column_width[0] = mwindow->session->keyframedialog_column1;
	column_width[1] = mwindow->session->keyframedialog_column2;
	if(selection >= 0 && selection < keyframe_data[0].size()) {
		for(int i = 0; i < KEYFRAME_COLUMNS; i++)
			keyframe_data[i].get(selection)->set_selected(1);
	}
	mwindow->gui->unlock_window();
}


void KeyFrameThread::start_window(Plugin *plugin, KeyFrame *keyframe)
{

	if(!BC_DialogThread::is_running()) {
		if(!mwindow->edl->tracks->plugin_exists(plugin)) return;
		this->keyframe = keyframe;
		this->plugin = plugin;
		plugin->calculate_title(plugin_title, 0);
		sprintf(window_title, _(PROGRAM_NAME ": %s Keyframe"), plugin_title);
		update_values();
		mwindow->gui->unlock_window();
		BC_DialogThread::start();
		mwindow->gui->lock_window("KeyFrameThread::start_window");
	}
	else {
		BC_DialogThread::start();
	}
}

BC_Window* KeyFrameThread::new_gui()
{
	mwindow->gui->lock_window("KeyFrameThread::new_gui");
	
	int x = mwindow->gui->get_abs_cursor_x(0) - 
		mwindow->session->plugindialog_w / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) - 
		mwindow->session->plugindialog_h / 2;

	KeyFrameWindow *window = new KeyFrameWindow(mwindow, 
		this, x, y, window_title);
	window->create_objects();

	mwindow->gui->unlock_window();
	return window;
}

void KeyFrameThread::handle_done_event(int result)
{
	if( !result )
		apply_value();
}

void KeyFrameThread::handle_close_event(int result)
{
	plugin = 0;
	keyframe = 0;
}


void KeyFrameThread::update_gui(int update_value_text)
{
	if(BC_DialogThread::is_running()) {
		mwindow->gui->lock_window("KeyFrameThread::update_gui");
		update_values();
		mwindow->gui->unlock_window();

		lock_window("KeyFrameThread::update_gui");
		KeyFrameWindow *window = (KeyFrameWindow*)get_gui();
		if(window) {
			window->lock_window("KeyFrameThread::update_gui");
			window->keyframe_list->update(keyframe_data,
				(const char **)column_titles,
				column_width,
				KEYFRAME_COLUMNS,
				window->keyframe_list->get_xposition(),
				window->keyframe_list->get_yposition(),
				window->keyframe_list->get_highlighted_item());
			if( update_value_text ) {
				int selection_number = window->keyframe_list->get_selection_number(0, 0);
				if( selection_number >= 0 && selection_number < keyframe_data[1].size()) {
					char *edit_value = keyframe_data[1].get(selection_number)->get_text();
					window->value_text->update(edit_value);
				}
			}
			window->unlock_window();
		}
		unlock_window();
	}
}

void KeyFrameThread::apply_value()
{
	const char *text = 0;
	BC_Hash hash;
	KeyFrameWindow *window = (KeyFrameWindow*)get_gui();
	int selection = window->keyframe_list->get_selection_number(0, 0);
//printf("KeyFrameThread::apply_value %d %d\n", __LINE__, selection);
	if(selection < 0) return;
	
	if(selection == keyframe_data[0].size() - 1)
		text = window->value_text->get_text();
	else {
		char *key = keyframe_data[0].get(selection)->get_text();
		const char *value = window->value_text->get_text();
		hash.update(key, value);
	}

	get_gui()->unlock_window();
	mwindow->gui->lock_window("KeyFrameThread::apply_value");
	if(plugin && mwindow->edl->tracks->plugin_exists(plugin)) {
		mwindow->undo->update_undo_before();
		if(mwindow->session->keyframedialog_all) {
// Search for all keyframes in selection but don't create a new one.
			Track *track = plugin->track;
			int64_t start = track->to_units(mwindow->edl->local_session->get_selectionstart(0), 0);
			int64_t end = track->to_units(mwindow->edl->local_session->get_selectionend(0), 0);
			int got_it = 0;
			for(KeyFrame *current = (KeyFrame*)plugin->keyframes->last;
				current;
				current = (KeyFrame*)PREVIOUS) {
				got_it = 1;
				if(current && current->position < end) {
					current->update_parameter(&hash, text, 0);
// Stop at beginning of range
					if(current->position <= start) break;
				}
			}

			if(!got_it) {
				KeyFrame* keyframe = (KeyFrame*)plugin->keyframes->default_auto;
				keyframe->update_parameter(&hash, text, 0);
			}
		}
		else {
// Create new keyframe if enabled
			KeyFrame *keyframe = plugin->get_keyframe();
			keyframe->update_parameter(&hash, text, 0);
		}
	}
	else {
printf("KeyFrameThread::apply_value %d: plugin doesn't exist\n", __LINE__);
	}

	mwindow->save_backup();
	mwindow->undo->update_undo_after(_("edit keyframe"), LOAD_AUTOMATION); 

	mwindow->update_plugin_guis(0);
	mwindow->gui->draw_overlays(1);
	mwindow->sync_parameters(CHANGE_PARAMS);



	mwindow->gui->unlock_window();

	update_gui(0);

	get_gui()->lock_window("KeyFrameThread::apply_value");
}


KeyFrameWindow::KeyFrameWindow(MWindow *mwindow,
	KeyFrameThread *thread, int x, int y, char *title_string)
 : BC_Window(title_string, x, y,
	mwindow->session->keyframedialog_w, 
	mwindow->session->keyframedialog_h, 
	320, 240, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void KeyFrameWindow::create_objects()
{
	Theme *theme = mwindow->theme;

	theme->get_keyframedialog_sizes(this);
	thread->column_width[0] = mwindow->session->keyframedialog_column1;
	thread->column_width[1] = mwindow->session->keyframedialog_column2;
	lock_window("KeyFrameWindow::create_objects");

	add_subwindow(title1 = new BC_Title(theme->keyframe_list_x,
		theme->keyframe_list_y - 
			BC_Title::calculate_h(this, (char*)"Py", LARGEFONT) - 
			theme->widget_border,
		_("Keyframe parameters:"), LARGEFONT));
	add_subwindow(keyframe_list = new KeyFrameList(thread,
		this, theme->keyframe_list_x, theme->keyframe_list_y,
		theme->keyframe_list_w, theme->keyframe_list_h));
	add_subwindow(title3 = new BC_Title(theme->keyframe_value_x,
		theme->keyframe_value_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border,
		_("Edit value:")));
	add_subwindow(value_text = new KeyFrameValue(thread,
		this, theme->keyframe_value_x, theme->keyframe_value_y, theme->keyframe_value_w));
	add_subwindow(all_toggle = new KeyFrameAll(thread,
		this, theme->keyframe_all_x, theme->keyframe_all_y));

	add_subwindow(new KeyFrameParamsOK(thread, this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	unlock_window();
}

int KeyFrameWindow::resize_event(int w, int h)
{
	Theme *theme = mwindow->theme;
	mwindow->session->keyframedialog_w = w;
	mwindow->session->keyframedialog_h = h;
	theme->get_keyframedialog_sizes(this);

	title1->reposition_window(theme->keyframe_list_x,
		theme->keyframe_list_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
	title3->reposition_window(theme->keyframe_value_x,
		theme->keyframe_value_y - BC_Title::calculate_h(this, (char*)"P") - theme->widget_border);
	keyframe_list->reposition_window(theme->keyframe_list_x, theme->keyframe_list_y,
		theme->keyframe_list_w, theme->keyframe_list_h);
	value_text->reposition_window(theme->keyframe_value_x, theme->keyframe_value_y,
		theme->keyframe_value_w);
	all_toggle->reposition_window(theme->keyframe_all_x, theme->keyframe_all_y);

	return 0;
}

KeyFrameList::KeyFrameList(KeyFrameThread *thread,
	KeyFrameWindow *window, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT,
	thread->keyframe_data, (const char **)thread->column_titles,
	thread->column_width, KEYFRAME_COLUMNS)
{
	this->thread = thread;
	this->window = window;
}

int KeyFrameList::selection_changed()
{
	window->value_text->update(
		thread->keyframe_data[1].get(get_selection_number(0, 0))->get_text());
	return 0;
}

int KeyFrameList::handle_event()
{
	window->set_done(0);
	return 0;
}

int KeyFrameList::column_resize_event()
{
	thread->mwindow->session->keyframedialog_column1 = get_column_width(0);
	thread->mwindow->session->keyframedialog_column2 = get_column_width(1);
	return 1;
}


KeyFrameValue::KeyFrameValue(KeyFrameThread *thread,
	KeyFrameWindow *window, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, "")
{
	this->thread = thread;
	this->window = window;
}

int KeyFrameValue::handle_event()
{
	thread->update_values();
	return 0;
}


KeyFrameAll::KeyFrameAll(KeyFrameThread *thread,
	KeyFrameWindow *window, int x, int y)
 : BC_CheckBox(x, y, thread->mwindow->session->keyframedialog_all, 
	_("Apply to all selected keyframes"))
{
	this->thread = thread;
	this->window = window;
}

int KeyFrameAll::handle_event()
{
	thread->mwindow->session->keyframedialog_all = get_value();
	return 1;
}

KeyFrameParamsOK::KeyFrameParamsOK(KeyFrameThread *thread, KeyFrameWindow *window)
 : BC_OKButton(window)
{
	this->thread = thread;
	this->window = window;
}

int KeyFrameParamsOK::keypress_event()
{
	if( get_keypress() == RETURN ) {
		thread->apply_value();
		return 1;
	}
	return 0;
}

