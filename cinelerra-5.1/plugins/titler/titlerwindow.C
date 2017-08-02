
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

#include "bcdisplayinfo.h"
#include "bcdialog.h"
#include "bcsignals.h"
#include "browsebutton.h"
#include "clip.h"
#include "cstrdup.h"
#include "automation.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "language.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginserver.h"
#include "theme.h"
#include "track.h"
#include "titlerwindow.h"
#include "bcfontentry.h"

static const int timeunit_formats[] =
{
	TIME_HMS,
	TIME_SECONDS,
	TIME_HMSF,
	TIME_SAMPLES,
	TIME_SAMPLES_HEX,
	TIME_FRAMES,
	TIME_FEET_FRAMES
};

TitleWindow::TitleWindow(TitleMain *client)
 : PluginClientWindow(client,
	client->config.window_w, client->config.window_h, 100, 100, 1)
{
//printf("TitleWindow::TitleWindow %d %d %d\n", __LINE__, client->config.window_w, client->config.window_h);
	this->client = client;

	font_title = 0;
	font = 0;
	font_tumbler = 0;
	x_title = 0; title_x = 0;
	y_title = 0; title_y = 0;
	w_title = 0; title_w = 0;
	h_title = 0; title_h = 0;
	dropshadow_title = 0; dropshadow = 0;
	outline_title = 0;    outline = 0;
	stroker_title = 0;    stroker = 0;
	style_title = 0;
	italic = 0;
	bold = 0;
	drag = 0;
	cur_popup = 0;
	fonts_popup = 0;

	color_x = color_y = 0;
	outline_color_x = outline_color_y = 0;
	drag_dx = drag_dy = dragging = 0;
	cur_ibeam = -1;

	size_title = 0;
	size = 0;
	size_tumbler = 0;
	pitch_title = 0;
	pitch = 0;
	encoding_title = 0;
	encoding = 0;
	color_button = 0;
	color_thread = 0;
	outline_color_button = 0;
	outline_color_thread = 0;
	motion_title = 0;
	motion = 0;
	line_pitch = 0;
	loop = 0;
	fadein_title = 0;
	fade_in = 0;
	fadeout_title = 0;
	fade_out = 0;
	text_title = 0;
	text = 0;
	text_chars = 0;
	text_bfrsz = 0;
	justify_title = 0;
	left = 0;  center = 0;  right = 0;
	top = 0;   mid = 0;     bottom = 0;
	speed_title = 0;
	speed = 0;
	timecode = 0;
	timecode_format = 0;
	background = 0;
	background_path = 0;
	loop_playback = 0;
}

void TitleWindow::done_event(int result)
{
	ungrab(client->server->mwindow->cwindow->gui);
	color_thread->close_window();
	outline_color_thread->close_window();
	color_popup->close_window();
	png_popup->close_window();
}

TitleWindow::~TitleWindow()
{
	delete color_popup;
	delete png_popup;
	for( int i=0; i<fonts.size(); ++i )
		delete fonts[i]->get_icon();

	sizes.remove_all_objects();
	delete timecode_format;
	delete color_thread;
	delete title_x;
	delete title_y;
}

void TitleWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = client->get_theme()->widget_border;
	char string[BCTEXTLEN];

#define COLOR_W 50
#define COLOR_H 30
	client->build_previews(this);

	sizes.append(new BC_ListBoxItem("8"));
	sizes.append(new BC_ListBoxItem("9"));
	sizes.append(new BC_ListBoxItem("10"));
	sizes.append(new BC_ListBoxItem("11"));
	sizes.append(new BC_ListBoxItem("12"));
	sizes.append(new BC_ListBoxItem("13"));
	sizes.append(new BC_ListBoxItem("14"));
	sizes.append(new BC_ListBoxItem("16"));
	sizes.append(new BC_ListBoxItem("18"));
	sizes.append(new BC_ListBoxItem("20"));
	sizes.append(new BC_ListBoxItem("22"));
	sizes.append(new BC_ListBoxItem("24"));
	sizes.append(new BC_ListBoxItem("26"));
	sizes.append(new BC_ListBoxItem("28"));
	sizes.append(new BC_ListBoxItem("32"));
	sizes.append(new BC_ListBoxItem("36"));
	sizes.append(new BC_ListBoxItem("40"));
	sizes.append(new BC_ListBoxItem("48"));
	sizes.append(new BC_ListBoxItem("56"));
	sizes.append(new BC_ListBoxItem("64"));
	sizes.append(new BC_ListBoxItem("72"));
	sizes.append(new BC_ListBoxItem("100"));
	sizes.append(new BC_ListBoxItem("128"));
	sizes.append(new BC_ListBoxItem("256"));
	sizes.append(new BC_ListBoxItem("512"));
	sizes.append(new BC_ListBoxItem("1024"));

	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(NO_MOTION)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(BOTTOM_TO_TOP)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(TOP_TO_BOTTOM)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(RIGHT_TO_LEFT)));
	paths.append(new BC_ListBoxItem(TitleMain::motion_to_text(LEFT_TO_RIGHT)));



// Construct font list
	ArrayList<BC_FontEntry*> *fontlist = get_resources()->fontlist;

	for( int i=0; i<fontlist->size(); ++i ) {
		int exists = 0;
		for( int j=0; j<fonts.size(); ++j ) {
			if( !strcasecmp(fonts.get(j)->get_text(),
				fontlist->get(i)->displayname) ) {
				exists = 1;
				break;
			}
		}

		BC_ListBoxItem *item = 0;
		if( !exists ) {
			fonts.append(item = new
				BC_ListBoxItem(fontlist->get(i)->displayname));
			if( !strcmp(client->config.font, item->get_text()) )
				item->set_selected(1);
			if( fontlist->values[i]->image ) {
				VFrame *vframe = fontlist->get(i)->image;
				BC_Pixmap *icon = new BC_Pixmap(this, vframe, PIXMAP_ALPHA);
				item->set_icon(icon);
				item->set_icon_vframe(vframe);
			}
		}
	}

// Sort font list
	int done = 0;
	while(!done) {
		done = 1;
		for( int i=0; i<fonts.size()-1; ++i ) {
			if( strcmp(fonts.values[i]->get_text(),
				fonts.values[i + 1]->get_text()) > 0 ) {
				BC_ListBoxItem *temp = fonts.values[i + 1];
				fonts.values[i + 1] = fonts.values[i];
				fonts.values[i] = temp;
				done = 0;
			}
		}
	}

	add_tool(font_title = new BC_Title(x, y, _("Font:")));
	font = new TitleFont(client, this, x, y + font_title->get_h());
	font->create_objects();
	font->set_show_query(1);
	x += font->get_w();
	add_subwindow(font_tumbler = new TitleFontTumble(client, this, x, y+margin));
	x += font_tumbler->get_w() + margin;

	int x1 = x, y1 = y;
	add_tool(size_title = new BC_Title(x1, y1+margin, _("Size:")));
	sprintf(string, "%.2f", client->config.size);
	x1 += size_title->get_w() + margin;
	size = new TitleSize(client, this, x1, y1+margin, string);
	size->create_objects();
	int x2 = x1 + size->get_w(), y2 = y1 + size->get_h() + margin;
	add_subwindow(size_tumbler = new TitleSizeTumble(client, this, x2, y1+margin));

	add_tool(pitch_title = new BC_Title(x-5, y2+margin, _("Pitch:")));
	pitch = new TitlePitch(client, this, x1, y2+margin, &client->config.line_pitch);
	pitch->create_objects();

	int x3 = x2 + size_tumbler->get_w() + 50;
	int y3 = pitch->get_y() + pitch->get_h();

	add_tool(style_title = new BC_Title(x=x3, y, _("Style:")));
	add_tool(italic = new TitleItalic(client, this, x, y + 20));
	int w1 = italic->get_w();
	add_tool(bold = new TitleBold(client, this, x, y + 50));
	if( bold->get_w() > w1 ) w1 = bold->get_w();
	add_tool(drag = new TitleDrag(client, this, x, y + 80));
	if( drag->get_w() > w1 ) w1 = drag->get_w();
	if( client->config.drag )
		grab(client->server->mwindow->cwindow->gui);

	x += w1 + margin;
	add_tool(justify_title = new BC_Title(x, y, _("Justify:")));
	add_tool(left = new TitleLeft(client, this, x, y + 20));
	w1 = left->get_w();
	add_tool(center = new TitleCenter(client, this, x, y + 50));
	if( center->get_w() > w1 ) w1 = center->get_w();
	add_tool(right = new TitleRight(client, this, x, y + 80));
	if( right->get_w() > w1 ) w1 = right->get_w();

	x += w1 + margin;
	add_tool(top = new TitleTop(client, this, x, y + 20));
	add_tool(mid = new TitleMid(client, this, x, y + 50));
	add_tool(bottom= new TitleBottom(client, this, x, y + 80));

	x = margin;
	y = y3+10;

	w1 = BC_Title::calculate_w(this, _("X:"));
	if( (x1 = BC_Title::calculate_w(this, _("Y:"))) > w1 ) w1 = x1;
	if( (x1 = BC_Title::calculate_w(this, _("W:"))) > w1 ) w1 = x1;
	if( (x1 = BC_Title::calculate_w(this, _("H:"))) > w1 ) w1 = x1;
	add_tool(x_title = new BC_Title(x1=x, y, _("X:")));
	x1 += w1;
	title_x = new TitleX(client, this, x1, y);
	title_x->create_objects();
	x1 += title_x->get_w()+margin;
	add_tool(y_title = new BC_Title(x1, y, _("Y:")));
	x1 += w1;
	title_y = new TitleY(client, this, x1, y);
	title_y->create_objects();
	x1 += title_y->get_w();
	y1 = y + title_y->get_h();

	add_tool(w_title = new BC_Title(x1=x, y1, _("W:")));
	x1 += w1;
	title_w = new TitleW(client, this, x1, y1);
	title_w->create_objects();
	x1 += title_w->get_w()+margin;
	add_tool(h_title = new BC_Title(x1, y1, _("H:")));
	x1 += w1;
	title_h = new TitleH(client, this, x1, y1);
	title_h->create_objects();
	x1 += title_h->get_w();

	x = x1+2*margin;
	add_tool(motion_title = new BC_Title(x1=x, y, _("Motion:")));
	x1 += motion_title->get_w()+margin;
	motion = new TitleMotion(client, this, x1, y);
	motion->create_objects();
	add_tool(loop = new TitleLoop(client, x, y1));
	x = margin;    y = y1 + loop->get_h()+20;

	add_tool(dropshadow_title = new BC_Title(x, y, _("Drop shadow:")));
	w1 = dropshadow_title->get_w();
	dropshadow = new TitleDropShadow(client, this, x, y + 20);
	dropshadow->create_objects();
	if( dropshadow->get_w() > w1 ) w1 = dropshadow->get_w();
	x += w1 + margin;

	add_tool(fadein_title = new BC_Title(x, y, _("Fade in (sec):")));
	w1 = fadein_title->get_w();
	add_tool(fade_in = new TitleFade(client, this, &client->config.fade_in, x, y + 20));
	if( fade_in->get_w() > w1 ) w1 = fade_in->get_w();
	x += w1 + margin;

	add_tool(fadeout_title = new BC_Title(x, y, _("Fade out (sec):")));
	w1 = fadeout_title->get_w();
	add_tool(fade_out = new TitleFade(client, this, &client->config.fade_out, x, y + 20));
	if( fade_out->get_w() > w1 ) w1 = fade_out->get_w();
	x += w1 + margin;

	add_tool(speed_title = new BC_Title(x, y1=y, _("Speed:")));
	w1 = speed_title->get_w();
	y += speed_title->get_h() + 5;
	speed = new TitleSpeed(client, this, x, y);
	speed->create_objects();
	if( speed->get_w() > w1 ) w1 = speed->get_w();
	x += w1 + margin;
	y2 = y + speed->get_h() + 10;

	color_x = x3;  color_y = y = y1;
	color_thread = new TitleColorThread(client, this, 0);
	x1 = color_x + COLOR_W + 2*margin;
	y1 = color_y + 5;
	add_tool(color_button = new TitleColorButton(client, this, x1, y1));
	y += COLOR_H + 5;
	outline_color_x = x3;  outline_color_y = y;
	outline_color_thread = new TitleColorThread(client, this, 1);
	y1 = outline_color_y + 5;
	add_tool(outline_color_button = new TitleOutlineColorButton(client, this, x1, y1));

	x = 10;  y = y2;
	add_tool(outline_title = new BC_Title(x, y, _("Outline:")));
	y1 =  y + outline_title->get_h() + margin;
	outline = new TitleOutline(client, this, x, y1);
	outline->create_objects();
	x += outline->get_w() + 2*margin;
#ifdef USE_STROKER
	add_tool(stroker_title = new BC_Title(x, y, _("Stroker:")));
	stroker = new TitleStroker(client, this, x, y1);
	stroker->create_objects();
	x += stroker->get_w() + margin;
#endif
	y += outline_title->get_h() + margin;
	add_tool(timecode = new TitleTimecode(client, x1=x, y));
	x += timecode->get_w() + margin;
	add_tool(timecode_format = new TitleTimecodeFormat(client, x, y,
		Units::print_time_format(client->config.timecode_format, string)));
	timecode_format->create_objects();
	y += timecode_format->get_h() + margin;

	x = 10;
	add_tool(background = new TitleBackground(client, this, x, y));
	x += background->get_w() + margin;
	add_tool(background_path = new TitleBackgroundPath(client, this, x, y));
	x += background_path->get_w() + margin;
	add_tool(background_browse = new BrowseButton(
		client->server->mwindow->theme, this, background_path,
		x, y, "", _("background media"), _("Select background media path")));
	x += background_browse->get_w() + 3*margin;
	add_tool(loop_playback = new TitleLoopPlayback(client, x, y));
	y += loop_playback->get_h() + 10;

	x = 10;
	add_tool(text_title = new BC_Title(x, y, _("Text:")));
	x += text_title->get_w() + 20;
	int wid = BC_Title::calculate_w(this,"0")*10;
	add_tool(text_chars = new TitleTextChars(x,y,wid));
	x += text_chars->get_w() + 20;
	add_tool(text_bfrsz = new TitleTextBfrSz(x,y,wid));

	y += text_title->get_h() + margin;
	x = margin;
	text = new TitleText(client, this, x, y, get_w()-margin - x, get_h() - y - 10);
	text->create_objects();

	add_tool(cur_popup = new TitleCurPopup(client, this));
	cur_popup->create_objects();
	add_tool(fonts_popup = new TitleFontsPopup(client, this));
	color_popup = new TitleColorPopup(client, this);
	png_popup = new TitlePngPopup(client, this);

	show_window(1);
	update();
}

int TitleWindow::resize_event(int w, int h)
{
	client->config.window_w = w;
	client->config.window_h = h;

	clear_box(0, 0, w, h);
	font_title->reposition_window(font_title->get_x(), font_title->get_y());
	font->reposition_window(font->get_x(), font->get_y());
	font_tumbler->reposition_window(font_tumbler->get_x(), font_tumbler->get_y());
	x_title->reposition_window(x_title->get_x(), x_title->get_y());
	title_x->reposition_window(title_x->get_x(), title_x->get_y());
	y_title->reposition_window(y_title->get_x(), y_title->get_y());
	title_y->reposition_window(title_y->get_x(), title_y->get_y());
	w_title->reposition_window(w_title->get_x(), w_title->get_y());
	title_w->reposition_window(title_w->get_x(), title_w->get_y());
	h_title->reposition_window(h_title->get_x(), h_title->get_y());
	title_h->reposition_window(title_h->get_x(), title_h->get_y());
	style_title->reposition_window(style_title->get_x(), style_title->get_y());
	italic->reposition_window(italic->get_x(), italic->get_y());
	bold->reposition_window(bold->get_x(), bold->get_y());
	drag->reposition_window(drag->get_x(), drag->get_y());
	size_title->reposition_window(size_title->get_x(), size_title->get_y());
	size->reposition_window(size->get_x(), size->get_y());
	size_tumbler->reposition_window(size_tumbler->get_x(), size_tumbler->get_y());
	pitch_title->reposition_window(pitch_title->get_x(), pitch_title->get_y());
	pitch->reposition_window(pitch->get_x(), pitch->get_y());

	color_button->reposition_window(color_button->get_x(), color_button->get_y());
	outline_color_button->reposition_window(outline_color_button->get_x(), outline_color_button->get_y());
	motion_title->reposition_window(motion_title->get_x(), motion_title->get_y());
	motion->reposition_window(motion->get_x(), motion->get_y());
	loop->reposition_window(loop->get_x(), loop->get_y());
	dropshadow_title->reposition_window(dropshadow_title->get_x(), dropshadow_title->get_y());
	dropshadow->reposition_window(dropshadow->get_x(), dropshadow->get_y());
	fadein_title->reposition_window(fadein_title->get_x(), fadein_title->get_y());
	fade_in->reposition_window(fade_in->get_x(), fade_in->get_y());
	fadeout_title->reposition_window(fadeout_title->get_x(), fadeout_title->get_y());
	fade_out->reposition_window(fade_out->get_x(), fade_out->get_y());
	text_title->reposition_window(text_title->get_x(), text_title->get_y());
	timecode->reposition_window(timecode->get_x(), timecode->get_y());
	text->reposition_window(text->get_x(), text->get_y(), w - text->get_x() - 10,
		BC_TextBox::pixels_to_rows(this, MEDIUMFONT, h - text->get_y() - 10));
	justify_title->reposition_window(justify_title->get_x(), justify_title->get_y());
	left->reposition_window(left->get_x(), left->get_y());
	center->reposition_window(center->get_x(), center->get_y());
	right->reposition_window(right->get_x(), right->get_y());
	top->reposition_window(top->get_x(), top->get_y());
	mid->reposition_window(mid->get_x(), mid->get_y());
	bottom->reposition_window(bottom->get_x(), bottom->get_y());
	speed_title->reposition_window(speed_title->get_x(), speed_title->get_y());
	speed->reposition_window(speed->get_x(), speed->get_y());
	update_color();
	flash();

	return 1;
}

int TitleWindow::grab_event(XEvent *event)
{
	MWindow *mwindow = client->server->mwindow;
	CWindowGUI *cwindow_gui = mwindow->cwindow->gui;
	CWindowCanvas *canvas = cwindow_gui->canvas;
	int cx, cy;  canvas->get_canvas()->get_relative_cursor_xy(cx, cy);
	if( cx < mwindow->theme->ccanvas_x ) return 0;
	if( cx >= mwindow->theme->ccanvas_x+mwindow->theme->ccanvas_w ) return 0;
	if( cy < mwindow->theme->ccanvas_y ) return 0;
	if( cy >= mwindow->theme->ccanvas_y+mwindow->theme->ccanvas_h ) return 0;

	switch( event->type ) {
	case ButtonPress:
		if( !dragging ) break;
		return 1;
	case ButtonRelease:
		if( !dragging ) return 0;
		dragging = 0;
		return 1;
	case MotionNotify:
		if( dragging ) break;
	default:
		return 0;
	}

	float cursor_x = cx, cursor_y = cy;
	canvas->canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
	int64_t position = client->get_source_position();
	float projector_x, projector_y, projector_z;
	Track *track = client->server->plugin->track;
	int track_w = track->track_w, track_h = track->track_h;
	track->automation->get_projector(
		&projector_x, &projector_y, &projector_z,
		position, PLAY_FORWARD);
	projector_x += mwindow->edl->session->output_w / 2;
	projector_y += mwindow->edl->session->output_h / 2;
	cursor_x = (cursor_x - projector_x) / projector_z + track_w / 2;
	cursor_y = (cursor_y - projector_y) / projector_z + track_h / 2;
	int title_x = client->config.title_x, title_y = client->config.title_y;
	int title_w = client->config.title_w, title_h = client->config.title_h;
	if( !title_w ) title_w = track_w;
	if( !title_h ) title_h = track_h;
	int r = MIN(track_w, track_h)/100 + 2;
	int x0 = title_x, x1 = title_x+(title_w+1)/2, x2 = title_x+title_w;
	int y0 = title_y, y1 = title_y+(title_h+1)/2, y2 = title_y+title_h;
	int drag_dx = 0, drag_dy = 0;
	if( !dragging ) {  // clockwise
		     if( abs(drag_dx = cursor_x-x0) < r &&  // x0,y0
			 abs(drag_dy = cursor_y-y0) < r ) dragging = 1;
		else if( abs(drag_dx = cursor_x-x1) < r &&  // x1,y0
			 abs(drag_dy = cursor_y-y0) < r ) dragging = 2;
		else if( abs(drag_dx = cursor_x-x2) < r &&  // x2,y0
			 abs(drag_dy = cursor_y-y0) < r ) dragging = 3;
		else if( abs(drag_dx = cursor_x-x2) < r &&  // x2,y1
			 abs(drag_dy = cursor_y-y1) < r ) dragging = 4;
		else if( abs(drag_dx = cursor_x-x2) < r &&  // x2,y2
			 abs(drag_dy = cursor_y-y2) < r ) dragging = 5;
		else if( abs(drag_dx = cursor_x-x1) < r &&  // x1,y2
			 abs(drag_dy = cursor_y-y2) < r ) dragging = 6;
		else if( abs(drag_dx = cursor_x-x0) < r &&  // x0,y2
			 abs(drag_dy = cursor_y-y2) < r ) dragging = 7;
		else if( abs(drag_dx = cursor_x-x0) < r &&  // x0,y1
			 abs(drag_dy = cursor_y-y1) < r ) dragging = 8;
		else if( abs(drag_dx = cursor_x-x1) < r &&  // x1,y1
			 abs(drag_dy = cursor_y-y1) < r ) dragging = 9;
			return 0;
	}
	switch( dragging ) {
	case 1: { // x0,y0
		int cur_x = cursor_x - drag_dx, dx = cur_x - x0;
		int cur_y = cursor_y - drag_dy, dy = cur_y - y0;
		if( !dx && !dy ) return 1;
		int cur_w = title_w - dx;  if( cur_w < 1 ) cur_w = 1;
		int cur_h = title_h - dy;  if( cur_h < 1 ) cur_h = 1;
		this->title_x->update((int64_t)(client->config.title_x = cur_x));
		this->title_y->update((int64_t)(client->config.title_y = cur_y));
		this->title_w->update((int64_t)(client->config.title_w = cur_w));
		this->title_h->update((int64_t)(client->config.title_h = cur_h));
		break; }
	case 2: { // x1,y0
		int cur_y = cursor_y - drag_dy, dy = cur_y - y0;
		if( !dy ) return 1;
		int cur_h = title_h - dy;  if( cur_h < 1 ) cur_h = 1;
		this->title_y->update((int64_t)(client->config.title_y = cur_y));
		this->title_h->update((int64_t)(client->config.title_h = cur_h));
		break; }
	case 3: { // x2,y0
		int cur_x = cursor_x - drag_dx, dx = cur_x - x2;
		int cur_y = cursor_y - drag_dy, dy = cur_y - y0;
		int cur_w = title_w + dx;  if( cur_w < 1 ) cur_w = 1;
		int cur_h = title_h - dy;  if( cur_h < 1 ) cur_h = 1;
		this->title_w->update((int64_t)(client->config.title_w = cur_w));
		this->title_y->update((int64_t)(client->config.title_y = cur_y));
		this->title_h->update((int64_t)(client->config.title_h = cur_h));
		break; }
	case 4: { // x2,y1
		int cur_x = cursor_x - drag_dx, dx = cur_x - x2;
		if( !dx ) return 1;
		int cur_w = title_w + dx;  if( cur_w < 1 ) cur_w = 1;
		this->title_w->update((int64_t)(client->config.title_w = cur_w));
		break; }
	case 5: { // x2,y2
		int cur_x = cursor_x - drag_dx, dx = cur_x - x2;
		int cur_y = cursor_y - drag_dy, dy = cur_y - y2;
		int cur_w = title_w + dx;  if( cur_w < 1 ) cur_w = 1;
		int cur_h = title_h + dy;  if( cur_h < 1 ) cur_h = 1;
		this->title_w->update((int64_t)(client->config.title_w = cur_w));
		this->title_h->update((int64_t)(client->config.title_h = cur_h));
		break; }
	case 6: { // x1,y2
		int cur_y = cursor_y - drag_dy, dy = cur_y - y2;
		if( client->config.title_h == cur_y ) return 1;
		int cur_h = title_h + dy;  if( cur_h < 1 ) cur_h = 1;
		this->title_h->update((int64_t)(client->config.title_h = cur_h));
		break; }
	case 7: { // x0,y2
		int cur_x = cursor_x - drag_dx, dx = cur_x - x0;
		int cur_y = cursor_y - drag_dy, dy = cur_y - y2;
		int cur_w = title_w - dx;  if( cur_w < 1 ) cur_w = 1;
		int cur_h = title_h + dy;  if( cur_h < 1 ) cur_h = 1;
		this->title_x->update((int64_t)(client->config.title_x = cur_x));
		this->title_w->update((int64_t)(client->config.title_w = cur_w));
		this->title_h->update((int64_t)(client->config.title_h = cur_h));
		break; }
	case 8: { // x0,y1
		int cur_x = cursor_x - drag_dx, dx = cur_x - x0;
		if( !dx ) return 1;
		int cur_w = title_w - dx;  if( cur_w < 1 ) cur_w = 1;
		this->title_x->update((int64_t)(client->config.title_x = cur_x));
		this->title_w->update((int64_t)(client->config.title_w = cur_w));
		break; }
	case 9: { // x1,y1
		int cur_x = cursor_x - drag_dx, dx = cur_x - x1;
		int cur_y = cursor_y - drag_dy, dy = cur_y - y1;
		if( title_x == cur_x && title_y == cur_y ) return 1;
		this->title_x->update((int64_t)(client->config.title_x += dx));
		this->title_y->update((int64_t)(client->config.title_y += dy));
		}
	}
	client->send_configure_change();
	return 1;
}

void TitleWindow::previous_font()
{
	int current_font = font->get_number();
	current_font--;
	if( current_font < 0 ) current_font = fonts.total - 1;

	if( current_font < 0 || current_font >= fonts.total ) return;

	for( int i=0; i<fonts.total; ++i ) {
		fonts.values[i]->set_selected(i == current_font);
	}

	font->update(fonts.values[current_font]->get_text());
	strcpy(client->config.font, fonts.values[current_font]->get_text());
	check_style(client->config.font,1);
	client->send_configure_change();
}

void  TitleWindow::next_font()
{
	int current_font = font->get_number();
	current_font++;
	if( current_font >= fonts.total ) current_font = 0;

	if( current_font < 0 || current_font >= fonts.total ) return;

	for( int i=0; i<fonts.total; ++i ) {
		fonts.values[i]->set_selected(i == current_font);
	}

	font->update(fonts.values[current_font]->get_text());
	strcpy(client->config.font, fonts.values[current_font]->get_text());
	check_style(client->config.font,1);
	client->send_configure_change();
}

int TitleWindow::insert_ibeam(const char *txt, int ofs)
{
	int ibeam = cur_ibeam;
	int ilen = strlen(txt)+1;
	wchar_t wtxt[ilen];
	int len = BC_Resources::encode(client->config.encoding, BC_Resources::wide_encoding,
		(char*)txt,ilen, (char *)wtxt,ilen*sizeof(wtxt[0])) / sizeof(wchar_t);
	client->insert_text(wtxt, ibeam);
	while( len > 0 && !wtxt[len] ) --len;
	int adv = len+1 + ofs;
	if( (ibeam += adv) >= client->config.wlen)
		ibeam = client->config.wlen;
	text->wset_selection(-1, -1, ibeam);
	text->update(client->config.wtext);
	client->send_configure_change();
	return 1;
}

void TitleWindow::update_color()
{
//printf("TitleWindow::update_color %x\n", client->config.color);
	set_color(client->config.color);
	draw_box(color_x, color_y, COLOR_W, COLOR_H);
	flash(color_x, color_y, COLOR_W, COLOR_H);
	set_color(client->config.outline_color);
	draw_box(outline_color_x, outline_color_y, COLOR_W, COLOR_H);
	set_color(BLACK);
	draw_rectangle(color_x, color_y, COLOR_W, COLOR_H);
	draw_rectangle(outline_color_x, outline_color_y, COLOR_W, COLOR_H);
	flash(outline_color_x, outline_color_y, COLOR_W, COLOR_H);
}

void TitleWindow::update_justification()
{
	left->update(client->config.hjustification == JUSTIFY_LEFT);
	center->update(client->config.hjustification == JUSTIFY_CENTER);
	right->update(client->config.hjustification == JUSTIFY_RIGHT);
	top->update(client->config.vjustification == JUSTIFY_TOP);
	mid->update(client->config.vjustification == JUSTIFY_MID);
	bottom->update(client->config.vjustification == JUSTIFY_BOTTOM);
}

void TitleWindow::update_stats()
{
	text_chars->update(client->config.wlen);
	int len = MESSAGESIZE - BCTEXTLEN - strlen(text->get_text()) - 1;
	if( len < 0 ) len = 0;
	text_bfrsz->update(len);
}

void TitleWindow::update()
{
	title_x->update((int64_t)client->config.title_x);
	title_y->update((int64_t)client->config.title_y);
	title_w->update((int64_t)client->config.title_w);
	title_h->update((int64_t)client->config.title_h);
	italic->update(client->config.style & BC_FONT_ITALIC);
	bold->update(client->config.style & BC_FONT_BOLD);
	size->update(client->config.size);
	motion->update(TitleMain::motion_to_text(client->config.motion_strategy));
	loop->update(client->config.loop);
	dropshadow->update((int64_t)client->config.dropshadow);
	fade_in->update((float)client->config.fade_in);
	fade_out->update((float)client->config.fade_out);
	font->update(client->config.font);
	check_style(client->config.font,0);
	text->update(&client->config.wtext[0]);
	speed->update(client->config.pixels_per_second);
	outline->update((int64_t)client->config.outline_size);
#ifdef USE_STROKER
	stroker->update((int64_t)client->config.stroke_width);
#endif
	timecode->update(client->config.timecode);
	timecode_format->update(client->config.timecode_format);
	background->update(client->config.background);
	background_path->update(client->config.background_path);
	loop_playback->update((int64_t)client->config.loop_playback);

	char string[BCTEXTLEN];
	for( int i=0; i<lengthof(timeunit_formats); ++i ) {
		if( timeunit_formats[i] == client->config.timecode_format ) {
			timecode_format->set_text(
				Units::print_time_format(timeunit_formats[i], string));
			break;
		}
	}
	update_justification();
	update_stats();
	update_color();
}


TitleFontTumble::TitleFontTumble(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Tumbler(x, y)
{
	this->client = client;
	this->window = window;
}
int TitleFontTumble::handle_up_event()
{
	window->previous_font();
	return 1;
}

int TitleFontTumble::handle_down_event()
{
	window->next_font();
	return 1;
}



TitleSizeTumble::TitleSizeTumble(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Tumbler(x, y)
{
	this->client = client;
	this->window = window;
}

int TitleSizeTumble::handle_up_event()
{
	int current_index = -1;
	int current_difference = -1;
	for( int i=0; i<window->sizes.size(); ++i ) {
		int size = atoi(window->sizes.get(i)->get_text());
		if( current_index < 0 ||
			abs(size - client->config.size) < current_difference ) {
			current_index = i;
			current_difference = abs(size - client->config.size);
		}
	}

	current_index++;
	if( current_index >= window->sizes.size() ) current_index = 0;


	client->config.size = atoi(window->sizes.get(current_index)->get_text());
	window->size->update(client->config.size);
	client->send_configure_change();
	return 1;
}

int TitleSizeTumble::handle_down_event()
{
	int current_index = -1;
	int current_difference = -1;
	for( int i=0; i<window->sizes.size(); ++i ) {
		int size = atoi(window->sizes.get(i)->get_text());
		if( current_index < 0 ||
			abs(size - client->config.size) < current_difference ) {
			current_index = i;
			current_difference = abs(size - client->config.size);
		}
	}

	current_index--;
	if( current_index < 0 ) current_index = window->sizes.size() - 1;


	client->config.size = atoi(window->sizes.get(current_index)->get_text());
	window->size->update(client->config.size);
	client->send_configure_change();
	return 1;
}

TitleBold::TitleBold(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.style & BC_FONT_BOLD, _("Bold"))
{
	this->client = client;
	this->window = window;
}

int TitleBold::handle_event()
{
	client->config.style =
		(client->config.style & ~BC_FONT_BOLD) |
			(get_value() ? BC_FONT_BOLD : 0);
	client->send_configure_change();
	return 1;
}

TitleItalic::TitleItalic(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.style & BC_FONT_ITALIC, _("Italic"))
{
	this->client = client;
	this->window = window;
}
int TitleItalic::handle_event()
{
	client->config.style =
		(client->config.style & ~BC_FONT_ITALIC) |
			(get_value() ? BC_FONT_ITALIC : 0);
	client->send_configure_change();
	return 1;
}



TitleSize::TitleSize(TitleMain *client, TitleWindow *window, int x, int y, char *text)
 : BC_PopupTextBox(window, &window->sizes, text, x, y, 64, 300)
{
	this->client = client;
	this->window = window;
}
TitleSize::~TitleSize()
{
}
int TitleSize::handle_event()
{
	client->config.size = atol(get_text());
//printf("TitleSize::handle_event 1 %s\n", get_text());
	client->send_configure_change();
	return 1;
}
void TitleSize::update(int size)
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", size);
	BC_PopupTextBox::update(string);
}

TitlePitch::
TitlePitch(TitleMain *client, TitleWindow *window, int x, int y, int *value)
 : BC_TumbleTextBox(window, *value, 0, INT_MAX, x, y, 64)
{
	this->client = client;
	this->window = window;
	this->value = value;
}

TitlePitch::
~TitlePitch()
{
}

int TitlePitch::handle_event()
{
	*value = atol(get_text());
	client->send_configure_change();
	return 1;
}

TitleColorButton::TitleColorButton(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Text Color..."))
{
	this->client = client;
	this->window = window;
}
int TitleColorButton::handle_event()
{
	window->color_thread->start_window(client->config.color,
		client->config.alpha);
	return 1;
}
TitleOutlineColorButton::TitleOutlineColorButton(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Outline color..."))
{
	this->client = client;
	this->window = window;
}
int TitleOutlineColorButton::handle_event()
{
	window->outline_color_thread->start_window(client->config.outline_color,
		client->config.outline_alpha);
	return 1;
}


TitleMotion::TitleMotion(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_PopupTextBox(window, &window->paths,
		client->motion_to_text(client->config.motion_strategy),
		x, y, 120, 100)
{
	this->client = client;
	this->window = window;
}
int TitleMotion::handle_event()
{
	client->config.motion_strategy = client->text_to_motion(get_text());
	client->send_configure_change();
	return 1;
}

TitleLoop::TitleLoop(TitleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.loop, _("Loop"))
{
	this->client = client;
}
int TitleLoop::handle_event()
{
	client->config.loop = get_value();
	client->send_configure_change();
	return 1;
}
TitleTimecode::TitleTimecode(TitleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.timecode, _("Stamp timecode"))
{
	this->client = client;
}
int TitleTimecode::handle_event()
{
	client->config.timecode = get_value();
	client->send_configure_change();
	return 1;
}

TitleTimecodeFormat::TitleTimecodeFormat(TitleMain *client, int x, int y, const char *text)
 : BC_PopupMenu(x, y, 100, text, 1)
{
	this->client = client;
}

int TitleTimecodeFormat::handle_event()
{
	client->config.timecode_format = Units::text_to_format(get_text());
	client->send_configure_change();
	return 1;
}

void TitleTimecodeFormat::create_objects()
{
	char string[BCTEXTLEN];
	for( int i=0; i<lengthof(timeunit_formats); ++i ) {
		add_item(new BC_MenuItem(
			Units::print_time_format(timeunit_formats[i], string)));
	}
}


int TitleTimecodeFormat::update(int timecode_format)
{
	char string[BCTEXTLEN];
	for( int i=0; i<lengthof(timeunit_formats); ++i ) {
		if( timeunit_formats[i] == timecode_format ) {
			set_text(Units::print_time_format(timeunit_formats[i], string));
			break;
		}
	}
	return 0;
}

TitleFade::TitleFade(TitleMain *client, TitleWindow *window,
	double *value, int x, int y)
 : BC_TextBox(x, y, 80, 1, (float)*value)
{
	this->client = client;
	this->window = window;
	this->value = value;
	set_precision(2);
}

int TitleFade::handle_event()
{
	*value = atof(get_text());
	client->send_configure_change();
	return 1;
}

void TitleWindow::check_style(const char *font_name, int update)
{
	BC_FontEntry *font_nrm = TitleMain::get_font(font_name, 0);
	BC_FontEntry *font_itl = TitleMain::get_font(font_name, BC_FONT_ITALIC);
	BC_FontEntry *font_bld = TitleMain::get_font(font_name, BC_FONT_BOLD);
	BC_FontEntry *font_bit = TitleMain::get_font(font_name, BC_FONT_ITALIC | BC_FONT_BOLD);
	int has_norm = font_nrm != 0 ? 1 : 0;
	int has_ital = font_itl != 0 || font_bit != 0 ? 1 : 0;
	int has_bold = font_bld != 0 || font_bit != 0 ? 1 : 0;
	if( bold->get_value() ) {
		if( !has_bold ) bold->update(0);
	}
	else {
		if( !has_norm && has_bold ) bold->update(1);
	}
	if( italic->get_value() ) {
		if( !has_ital ) italic->update(0);
	}
	else {
		if( !has_norm && has_ital ) italic->update(1);
	}
	if( has_norm && has_bold ) bold->enable();   else bold->disable();
	if( has_norm && has_ital ) italic->enable(); else italic->disable();
	if( update ) {
		int style = stroker && atof(stroker->get_text()) ? BC_FONT_OUTLINE : 0;
		if( bold->get_value() ) style |= BC_FONT_BOLD;
		if( italic->get_value() ) style |= BC_FONT_ITALIC;
		client->config.style = style;
	}
}

TitleFont::TitleFont(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_PopupTextBox(window, &window->fonts, client->config.font,
		x, y, 240, 300, LISTBOX_ICON_LIST)
{
	this->client = client;
	this->window = window;
}
int TitleFont::handle_event()
{
	strcpy(client->config.font, get_text());
	window->check_style(client->config.font, 1);
	client->send_configure_change();
	return 1;
}

TitleText::TitleText(TitleMain *client, TitleWindow *window,
	int x, int y, int w, int h)
 : BC_ScrollTextBox(window, x, y, w,
		BC_TextBox::pixels_to_rows(window, MEDIUMFONT, h),
		client->config.wtext, 8192)
{
	this->client = client;
	this->window = window;
//printf("TitleText::TitleText %s\n", client->config.text);
}

int TitleText::button_press_event()
{
	if( get_buttonpress() == 3 ) {
		window->cur_ibeam = get_ibeam_letter();
		window->cur_popup->activate_menu();
		return 1;
	}
	return BC_ScrollTextBox::button_press_event();
}

int TitleText::handle_event()
{
	window->fonts_popup->deactivate();
	int text_len = strlen(get_text());
	int avail = MESSAGESIZE - BCTEXTLEN;
	if( text_len >= avail ) { // back off last utf8 char
		char text[2*sizeof(client->config.wtext)];
		strcpy(text, get_text());
                text_len = avail;
                while( text_len > 0 && (text[text_len-1] & 0xc0) == 0x80 )
                        text[--text_len] = 0;
                if( text_len > 0 )
                        text[--text_len] = 0;
		update(text);
	}
	int len =  sizeof(client->config.wtext) / sizeof(wchar_t);
	wcsncpy(client->config.wtext, get_wtext(), len);
	client->config.wtext[len-1] = 0;
	client->config.wlen = wcslen(client->config.wtext);
	window->update_stats();
	client->send_configure_change();
	return 1;
}
TitleTextChars::TitleTextChars(int x, int y, int w)
 : BC_Title(x, y, "", MEDIUMFONT, -1, 0, w)
{
}
TitleTextChars::~TitleTextChars()
{
}
int TitleTextChars::update(int n)
{
	char text[BCSTRLEN];
	sprintf(text, _("chars: %d  "),n);
	return BC_Title::update(text, 0);
}

TitleTextBfrSz::TitleTextBfrSz(int x, int y, int w)
 : BC_Title(x, y, "", MEDIUMFONT, -1, 0, w)
{
}
TitleTextBfrSz::~TitleTextBfrSz()
{
}
int TitleTextBfrSz::update(int n)
{
	char text[BCSTRLEN];
	sprintf(text, _("bfrsz: %d  "),n);
	return BC_Title::update(text, 0);
}


TitleDropShadow::TitleDropShadow(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window, (int64_t)client->config.dropshadow,
	(int64_t)-1000, (int64_t)1000, x, y, 70)
{
	this->client = client;
	this->window = window;
}
int TitleDropShadow::handle_event()
{
	client->config.dropshadow = atol(get_text());
	client->send_configure_change();
	return 1;
}


TitleOutline::TitleOutline(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window, (int64_t)client->config.outline_size,
	(int64_t)0, (int64_t)1000, x, y, 70)
{
	this->client = client;
	this->window = window;
}
int TitleOutline::handle_event()
{
	client->config.outline_size = atol(get_text());
	client->send_configure_change();
	return 1;
}


TitleStroker::TitleStroker(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window, (int64_t)client->config.stroke_width,
	(int64_t)0, (int64_t)1000, x, y, 70)
{
	this->client = client;
	this->window = window;
}
int TitleStroker::handle_event()
{
	client->config.stroke_width = atof(get_text());
	if( client->config.stroke_width )
		client->config.style |= BC_FONT_OUTLINE;
	else
		client->config.style &= ~BC_FONT_OUTLINE;
	client->send_configure_change();
	return 1;
}


TitleX::TitleX(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window, (int64_t)client->config.title_x,
	(int64_t)-32767, (int64_t)32767, x, y, 50)
{
	this->client = client;
	this->window = window;
}
int TitleX::handle_event()
{
	client->config.title_x = atol(get_text());
	client->send_configure_change();
	return 1;
}

TitleY::TitleY(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window, (int64_t)client->config.title_y,
	(int64_t)-32767, (int64_t)32767, x, y, 50)
{
	this->client = client;
	this->window = window;
}
int TitleY::handle_event()
{
	client->config.title_y = atol(get_text());
	client->send_configure_change();
	return 1;
}

TitleW::TitleW(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window, (int64_t)client->config.title_w,
	(int64_t)0, (int64_t)32767, x, y, 50)
{
	this->client = client;
	this->window = window;
}
int TitleW::handle_event()
{
	client->config.title_w = atol(get_text());
	client->send_configure_change();
	return 1;
}

TitleH::TitleH(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window, (int64_t)client->config.title_h,
	(int64_t)0, (int64_t)32767, x, y, 50)
{
	this->client = client;
	this->window = window;
}
int TitleH::handle_event()
{
	client->config.title_h = atol(get_text());
	client->send_configure_change();
	return 1;
}

TitleSpeed::TitleSpeed(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TumbleTextBox(window, (float)client->config.pixels_per_second,
	(float)0, (float)1000, x, y, 100)
{
	this->client = client;
	set_precision(2);
	set_increment(10);
}


int TitleSpeed::handle_event()
{
	client->config.pixels_per_second = atof(get_text());
	client->send_configure_change();
	return 1;
}


TitleLeft::TitleLeft(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_LEFT, _("Left"))
{
	this->client = client;
	this->window = window;
}
int TitleLeft::handle_event()
{
	client->config.hjustification = JUSTIFY_LEFT;
	window->update_justification();
	client->send_configure_change();
	return 1;
}

TitleCenter::TitleCenter(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_CENTER, _("Center"))
{
	this->client = client;
	this->window = window;
}
int TitleCenter::handle_event()
{
	client->config.hjustification = JUSTIFY_CENTER;
	window->update_justification();
	client->send_configure_change();
	return 1;
}

TitleRight::TitleRight(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.hjustification == JUSTIFY_RIGHT, _("Right"))
{
	this->client = client;
	this->window = window;
}
int TitleRight::handle_event()
{
	client->config.hjustification = JUSTIFY_RIGHT;
	window->update_justification();
	client->send_configure_change();
	return 1;
}



TitleTop::TitleTop(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_TOP, _("Top"))
{
	this->client = client;
	this->window = window;
}
int TitleTop::handle_event()
{
	client->config.vjustification = JUSTIFY_TOP;
	window->update_justification();
	client->send_configure_change();
	return 1;
}

TitleMid::TitleMid(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_MID, _("Mid"))
{
	this->client = client;
	this->window = window;
}
int TitleMid::handle_event()
{
	client->config.vjustification = JUSTIFY_MID;
	window->update_justification();
	client->send_configure_change();
	return 1;
}

TitleBottom::TitleBottom(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_Radial(x, y, client->config.vjustification == JUSTIFY_BOTTOM, _("Bottom"))
{
	this->client = client;
	this->window = window;
}
int TitleBottom::handle_event()
{
	client->config.vjustification = JUSTIFY_BOTTOM;
	window->update_justification();
	client->send_configure_change();
	return 1;
}



TitleColorThread::TitleColorThread(TitleMain *client, TitleWindow *window, int is_outline)
 : ColorPicker(1, is_outline? _("Outline Color") : _("Text Color"))
{
	this->client = client;
	this->window = window;
	this->is_outline = is_outline;
}

int TitleColorThread::handle_new_color(int output, int alpha)
{
	if( is_outline ) {
		client->config.outline_color = output;
		client->config.outline_alpha = alpha;
	}
	else {
		client->config.color = output;
		client->config.alpha = alpha;
	}

	window->lock_window("TitleColorThread::handle_new_color");
	window->update_color();
	window->flush();
	window->unlock_window();

	client->send_configure_change();
	return 1;
}

TitleDrag::TitleDrag(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.drag, _("Drag"))
{
	this->client = client;
	this->window = window;
}

int TitleDrag::handle_event()
{
	int value = get_value();
	client->config.drag = value;
	if( value )
		window->grab(client->server->mwindow->cwindow->gui);
	else
		window->ungrab(client->server->mwindow->cwindow->gui);
	client->send_configure_change();
	return 1;
}

TitleBackground::TitleBackground(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_CheckBox(x, y, client->config.background, _("Background:"))
{
	this->client = client;
	this->window = window;
}

int TitleBackground::handle_event()
{
	client->config.background = get_value();
	client->send_configure_change();
	return 1;
}

TitleBackgroundPath::TitleBackgroundPath(TitleMain *client, TitleWindow *window, int x, int y)
 : BC_TextBox(x, y, 240, 1, client->config.background_path)
{
	this->client = client;
	this->window = window;
}

int TitleBackgroundPath::handle_event()
{
	strncpy(client->config.background_path, get_text(), sizeof(client->config.background_path));
	client->send_configure_change();
	return 1;
}

TitleLoopPlayback::TitleLoopPlayback(TitleMain *client, int x, int y)
 : BC_CheckBox(x, y, client->config.loop_playback, _("Loop playback"))
{
	this->client = client;
}
int TitleLoopPlayback::handle_event()
{
	client->config.loop_playback = get_value();
	client->send_configure_change();
	return 1;
}


TitleCurPopup::TitleCurPopup(TitleMain *client, TitleWindow *window)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->client = client;
	this->window = window;
}
int TitleCurPopup::handle_event()
{
	return 1;
}

void TitleCurSubMenu::add_subitemx(int popup_type, va_list ap, const char *fmt)
{
	char item[BCSTRLEN];
	vsnprintf(item, sizeof(item)-1, fmt, ap);
	item[sizeof(item)-1] = 0;
	add_submenuitem(new TitleCurSubMenuItem(this, item, popup_type));
}

void TitleCurPopup::create_objects()
{
	TitleCurItem *cur_item;
	TitleCurSubMenu *sub_menu;
	char *item;
	add_item(cur_item = new TitleCurItem(this, item = KW_NUDGE));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s dx,dy",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_COLOR));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem(POPUP_COLOR,"%s %s",item,_("#"));
	sub_menu->add_subitem("%s ",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_ALPHA));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s ",item);
	sub_menu->add_subitem("%s 0.",item);
	sub_menu->add_subitem("%s .5",item);
	sub_menu->add_subitem("%s 1.",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_FONT));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem(POPUP_FONT,"%s %s",item, _("name"));
	sub_menu->add_subitem(POPUP_OFFSET, "%s ",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_SIZE));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s +",item);
	sub_menu->add_subitem("%s -",item);
	sub_menu->add_subitem("%s ",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_BOLD));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s 1",item);
	sub_menu->add_subitem("%s 0",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_ITALIC));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s 1",item);
	sub_menu->add_subitem("%s 0",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_CAPS));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s 1",item);
	sub_menu->add_subitem("%s 0",item);
	sub_menu->add_subitem("%s -1",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_UL));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s 1",item);
	sub_menu->add_subitem("%s 0",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_BLINK));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s 1",item);
	sub_menu->add_subitem("%s -1",item);
	sub_menu->add_subitem("%s ",item);
	sub_menu->add_subitem("%s 0",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_FIXED));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s ",item);
	sub_menu->add_subitem("%s 20",item);
	sub_menu->add_subitem("%s 10",item);
	sub_menu->add_subitem("%s 0",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_SUP));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem("%s 1",item);
	sub_menu->add_subitem("%s 0",item);
	sub_menu->add_subitem("%s -1",item);
	sub_menu->add_subitem("/%s",item);
	add_item(cur_item = new TitleCurItem(this, item = KW_PNG));
	cur_item->add_submenu(sub_menu = new TitleCurSubMenu(cur_item));
	sub_menu->add_subitem(POPUP_PNG,"%s %s", item, _("file"));
}

TitleCurItem::TitleCurItem(TitleCurPopup *popup, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
}
int TitleCurItem::handle_event()
{
	return 1;
}

TitleCurSubMenu::TitleCurSubMenu(TitleCurItem *cur_item)
{
	this->cur_item = cur_item;
}
TitleCurSubMenu::~TitleCurSubMenu()
{
}

TitleCurSubMenuItem::TitleCurSubMenuItem(TitleCurSubMenu *submenu, const char *text, int popup_type)
 : BC_MenuItem(text)
{
	this->submenu = submenu;
	this->popup_type = popup_type;
}
TitleCurSubMenuItem::~TitleCurSubMenuItem()
{
}
int TitleCurSubMenuItem::handle_event()
{
	TitleCurPopup *popup = submenu->cur_item->popup;
	TitleWindow *window = popup->window;
	const char *item_text = get_text();
	int ofs = *item_text == '/' ? 0 : -1;
	switch( popup_type ) {
	case POPUP_FONT: {
		int px, py;
		window->get_pop_cursor_xy(px ,py);
		window->fonts_popup->activate(px, py, 300,200);
		return 1; }
	case POPUP_COLOR: {
		window->color_popup->activate();
		return 1; }
	case POPUP_PNG: {
		window->png_popup->activate();
		return 1; }
	case POPUP_OFFSET:
		ofs = -1;
		break;
	}
	char txt[BCSTRLEN];
	sprintf(txt, "<%s>", item_text);
	return window->insert_ibeam(txt, ofs);
}

TitleFontsPopup::TitleFontsPopup(TitleMain *client, TitleWindow *window)
 : BC_ListBox(-1, -1, 1, 1, LISTBOX_ICON_LIST,
	&window->fonts, 0, 0, 1, 0, 1)
{
	this->client = client;
	this->window = window;
	set_use_button(0);
	set_show_query(1);
}
TitleFontsPopup::~TitleFontsPopup()
{
}
int TitleFontsPopup::keypress_event()
{
	switch( get_keypress() ) {
	case ESC:
	case DELETE:
		deactivate();
		return 1;
	default:
		break;
	}
	return BC_ListBox::keypress_event();
}

int TitleFontsPopup::handle_event()
{
	deactivate();
	BC_ListBoxItem *item = get_selection(0, 0);
	if( !item ) return 1;
	const char *item_text = item->get_text();
	char txt[BCTEXTLEN];  sprintf(txt, "<%s %s>", KW_FONT, item_text);
	return window->insert_ibeam(txt);
}

TitleColorPopup::TitleColorPopup(TitleMain *client, TitleWindow *window)
 : ColorPicker(0, _("Color"))
{
	this->client = client;
	this->window = window;
	this->color_value = client->config.color;
}
TitleColorPopup::~TitleColorPopup()
{
}
int TitleColorPopup::handle_new_color(int output, int alpha)
{
	color_value = output;
	return 1;
}
int TitleColorPopup::activate()
{
	start_window(client->config.color, 255, 1);
	return 1;
}
void TitleColorPopup::handle_done_event(int result)
{
	if( result ) return;
	char txt[BCSTRLEN];  sprintf(txt, "<%s #%06x>", KW_COLOR, color_value);
	window->insert_ibeam(txt);
}

TitlePngPopup::TitlePngPopup(TitleMain *client, TitleWindow *window)
 : BC_DialogThread()
{
	this->client = client;
	this->window = window;
}

TitlePngPopup::~TitlePngPopup()
{
	close_window();
}

void TitlePngPopup::handle_done_event(int result)
{
	if( result ) return;
	BrowseButtonWindow *gui = (BrowseButtonWindow *)get_gui();
	const char *path = gui->get_submitted_path();
	char txt[BCSTRLEN];  sprintf(txt, "<%s %s>", KW_PNG, path);
	window->insert_ibeam(txt);
}

BC_Window *TitlePngPopup::new_gui()
{
        BC_DisplayInfo display_info;
        int x = display_info.get_abs_cursor_x();
        int y = display_info.get_abs_cursor_y();

	BC_Window *gui = new BrowseButtonWindow(client->server->mwindow->theme,
		x-25, y-100, window, "", _("Png file"), _("Png path"), 0);
        gui->create_objects();
	return gui;
}

int TitlePngPopup::activate()
{
	BC_DialogThread::start();
	return 1;
}

