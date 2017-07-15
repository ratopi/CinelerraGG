/*
 * CINELERRA
 * Copyright (C) 2006 Pierre Dumuid
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

#include "awindow.h"
#include "awindowgui.h"
#include "bcdialog.h"
#include "labeledit.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"


LabelEdit::LabelEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->awindow = awindow;
	this->vwindow = vwindow;
	label = 0;
	label_edit_window = 0;
}

LabelEdit::~LabelEdit()
{
	close_window();
}

void LabelEdit::start(Label *label)
{
	this->label = label;
	BC_DialogThread::start();
}

void LabelEdit::handle_close_event(int result)
{
	label_edit_window = 0;
}


void LabelEdit::handle_done_event(int result)
{
	awindow->gui->async_update_assets();
}

BC_Window *LabelEdit::new_gui()
{
 	int x = mwindow->gui->get_abs_cursor_x(1) - 400 / 2;
	int y = mwindow->gui->get_abs_cursor_y(1) - 350 / 2;
	label_edit_window = new LabelEditWindow(mwindow, this, x, y);
	label_edit_window->create_objects();
	return label_edit_window;
}

LabelEditWindow::LabelEditWindow(MWindow *mwindow, LabelEdit *thread, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Label Info"), x, y,
	400, 350, 400, 430, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

LabelEditWindow::~LabelEditWindow()
{
}

void LabelEditWindow::create_objects()
{
	this->label = thread->label;

	int x = 10, y = 10;
	int x1 = x;
	BC_TextBox *textbox;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x1, y, _("Label Text:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new LabelEditComments(this, x1, y, get_w() - x1 * 2,
		BC_TextBox::pixels_to_rows(this, MEDIUMFONT, get_h() - 10 - 40 - y)));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	textbox->activate();
}

LabelEditComments::LabelEditComments(LabelEditWindow *window, int x, int y, int w, int rows)
 : BC_TextBox(x, y, w, rows, window->label->textstr,  1, MEDIUMFONT, 1)
{
	this->window = window;
}

int LabelEditComments::handle_event()
{
	strcpy(window->label->textstr, get_text());
	return 1;
}

