
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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
#include "colorpicker.h"
#include "condition.h"
#include "language.h"
#include "mutex.h"
#include "mwindow.inc"
#include "cicolors.h"
#include "vframe.h"

#include <string.h>
#include <unistd.h>


ColorThread::ColorThread(int do_alpha, const char *title)
 : BC_DialogThread()
{
	this->title = title;
	this->do_alpha = do_alpha;
	this->do_okcancel = 0;
	this->output = BLACK;
	this->alpha = 255;
}

ColorThread::~ColorThread()
{
	close_window();
}

void ColorThread::start_window(int output, int alpha, int do_okcancel)
{
	if( running() ) {
		ColorWindow *gui = (ColorWindow *)get_gui();
		if( gui ) {
			gui->lock_window("ColorThread::start_window");
			gui->raise_window(1);
			gui->unlock_window();
		}
		return;
	}
	this->output = output;
	this->alpha = alpha;
	this->do_okcancel = do_okcancel;
	start();
}

BC_Window* ColorThread::new_gui()
{
	char window_title[BCTEXTLEN];
	strcpy(window_title, _(PROGRAM_NAME ": "));
	strcat(window_title, title ? title : _("Color Picker"));
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x() + 25;
	int y = display_info.get_abs_cursor_y() - 100;
	int w = 540, h = 290;
	if( do_alpha )
		h += 40 + PalletteAPH::calculate_h();
	if( do_okcancel )
		h += bmax(BC_OKButton::calculate_h(),BC_CancelButton::calculate_h());
	int root_w = display_info.get_root_w(), root_h = display_info.get_root_h();
	if( x+w > root_w ) x = root_w - w;
	if( y+h > root_h ) y = root_h - h;
	if( x < 0 ) x = 0;
	if( y < 0 ) y = 0;
	ColorWindow *gui = new ColorWindow(this, x, y, w, h, window_title);
	gui->create_objects();
	return gui;
}

void ColorThread::update_gui(int output, int alpha)
{
	ColorWindow *gui = (ColorWindow *)get_gui();
	if( !gui ) return;
	gui->lock_window();
	this->output = output;
	this->alpha = alpha;
	gui->change_values();
	gui->update_display();
	gui->unlock_window();
}

int ColorThread::handle_new_color(int output, int alpha)
{
	printf("ColorThread::handle_new_color undefined.\n");
	return 0;
}


ColorWindow::ColorWindow(ColorThread *thread, int x, int y, int w, int h, const char *title)
 : BC_Window(title, x, y, w, h, w, h, 0, 0, 1)
{
	this->thread = thread;
	wheel = 0;
	wheel_value = 0;
	output = 0;

	hue = 0; sat = 0; val = 0;
	red = 0; grn = 0; blu = 0;
	lum = 0; c_r = 0; c_b = 0;
	alpha = 0;

	hsv_h = 0;  hsv_s = 0;  hsv_v = 0;
	rgb_r = 0;  rgb_g = 0;  rgb_b = 0;
	yuv_y = 0;  yuv_u = 0;  yuv_v = 0;
	aph_a = 0;
}
ColorWindow::~ColorWindow()
{
	delete hsv_h;  delete hsv_s;  delete hsv_v;
	delete rgb_r;  delete rgb_g;  delete rgb_b;
	delete yuv_y;  delete yuv_u;  delete yuv_v;
	delete aph_a;
}

void ColorWindow::create_objects()
{
	int x0 = 10, y0 = 10;
	lock_window("ColorWindow::create_objects");
	change_values();

	int x = x0, y = y0;
	add_tool(wheel = new PaletteWheel(this, x, y));
	wheel->create_objects();

	x += 180;  add_tool(wheel_value = new PaletteWheelValue(this, x, y));
	wheel_value->create_objects();
	x = x0;
	y += 180;  add_tool(output = new PaletteOutput(this, x, y));
	output->create_objects();
	y += output->get_h() + 20;

	x += 240;
	add_tool(new BC_Title(x, y =y0, _("H:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, _("S:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, _("V:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=40, _("R:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, _("G:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, _("B:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=40, _("Y:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, _("U:"), SMALLFONT));
	add_tool(new BC_Title(x, y+=25, _("V:"), SMALLFONT));
	if( thread->do_alpha )
		add_tool(new BC_Title(x, y+=40, _("A:"), SMALLFONT));
	x += 24;
	add_tool(hue = new PaletteHue(this, x, y= y0));
	add_tool(sat = new PaletteSat(this, x, y+=25));
	add_tool(val = new PaletteVal(this, x, y+=25));
	add_tool(red = new PaletteRed(this, x, y+=40));
	add_tool(grn = new PaletteGrn(this, x, y+=25));
	add_tool(blu = new PaletteBlu(this, x, y+=25));
	add_tool(lum = new PaletteLum(this, x, y+=40));
	add_tool(c_r = new PaletteCr (this, x, y+=25));
	add_tool(c_b = new PaletteCb (this, x, y+=25));
	if( thread->do_alpha )
		add_tool(alpha = new PaletteAlpha(this, x, y+=40));

	x += hue->get_w() + 10;
	hsv_h = new PalletteHSV(this, x,y= y0, hsv.h, 0, 360);  hsv_h->create_objects();
	hsv_s = new PalletteHSV(this, x,y+=25, hsv.s, 0, 1);    hsv_s->create_objects();
	hsv_v = new PalletteHSV(this, x,y+=25, hsv.v, 0, 1);    hsv_v->create_objects();
	rgb_r = new PalletteRGB(this, x,y+=40, rgb.r, 0, 1);    rgb_r->create_objects();
	rgb_g = new PalletteRGB(this, x,y+=25, rgb.g, 0, 1);    rgb_g->create_objects();
	rgb_b = new PalletteRGB(this, x,y+=25, rgb.b, 0, 1);    rgb_b->create_objects();
	yuv_y = new PalletteYUV(this, x,y+=40, yuv.y, 0, 1);    yuv_y->create_objects();
	yuv_u = new PalletteYUV(this, x,y+=25, yuv.u, 0, 1);    yuv_u->create_objects();
	yuv_v = new PalletteYUV(this, x,y+=25, yuv.v, 0, 1);    yuv_v->create_objects();
	if( thread->do_alpha ) {
		aph_a = new PalletteAPH(this, x,y+=40, aph, 0, 1); aph_a->create_objects();
	}
	if( thread->do_okcancel ) {
		add_tool(new BC_OKButton(this));
		add_tool(new BC_CancelButton(this));
	}

	update_display();
	show_window(1);
	unlock_window();
}


void ColorWindow::change_values()
{
	float r = ((thread->output>>16) & 0xff) / 255.;
	float g = ((thread->output>>8)  & 0xff) / 255.;
	float b = ((thread->output>>0)  & 0xff) / 255.;
	rgb.r = r;  rgb.g = g;  rgb.b = b;
	aph = (float)thread->alpha / 255;
	update_rgb(rgb.r, rgb.g, rgb.b);
}


int ColorWindow::close_event()
{
	set_done(thread->do_okcancel ? 1 : 0);
	return 1;
}


void ColorWindow::update_rgb()
{
	update_rgb(rgb.r, rgb.g, rgb.b);
	update_display();
}
void ColorWindow::update_hsv()
{
	update_hsv(hsv.h, hsv.s, hsv.v);
	update_display();
}
void ColorWindow::update_yuv()
{
	update_yuv(yuv.y, yuv.u, yuv.v);
	update_display();
}

void ColorWindow::update_display()
{
	wheel->draw(wheel->oldhue, wheel->oldsaturation);
	wheel->oldhue = hsv.h;
	wheel->oldsaturation = hsv.s;
	wheel->draw(hsv.h, hsv.s);
	wheel->flash();
	wheel_value->draw(hsv.h, hsv.s, hsv.v);
	wheel_value->flash();
	output->draw();
	output->flash();

	hue->update((int)hsv.h);
	sat->update(hsv.s);
	val->update(hsv.v);

	red->update(rgb.r);
	grn->update(rgb.g);
	blu->update(rgb.b);

	lum->update(yuv.y);
	c_r->update(yuv.u);
	c_b->update(yuv.v);

	hsv_h->update(hsv.h);
	hsv_s->update(hsv.s);
	hsv_v->update(hsv.v);
	rgb_r->update(rgb.r);
	rgb_g->update(rgb.g);
	rgb_b->update(rgb.b);
	yuv_y->update(yuv.y);
	yuv_u->update(yuv.u);
	yuv_v->update(yuv.v);
	if( thread->do_alpha )
		aph_a->update(aph);
}

int ColorWindow::handle_event()
{
	int r = 255*rgb.r + 0.5, g = 255*rgb.g + 0.5, b = 255*rgb.b + 0.5;
	int result = (r << 16) | (g << 8) | (b << 0);
	thread->handle_new_color(result, (int)(255*aph + 0.5));
	return 1;
}


PaletteWheel::PaletteWheel(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 170, 170)
{
	this->window = window;
	oldhue = 0;
	oldsaturation = 0;
	button_down = 0;
}

PaletteWheel::~PaletteWheel()
{
}

int PaletteWheel::button_press_event()
{
	if( get_cursor_x() >= 0 && get_cursor_x() < get_w() &&
		get_cursor_y() >= 0 && get_cursor_y() < get_h() &&
		is_event_win() ) {
		button_down = 1;
		cursor_motion_event();
		return 1;
	}
	return 0;
}

int PaletteWheel::cursor_motion_event()
{
	int x1, y1, distance;
	if( button_down && is_event_win() ) {
		float h = get_angle(get_w()/2, get_h()/2, get_cursor_x(), get_cursor_y());
		bclamp(h, 0, 359.999);  window->hsv.h = h;
		x1 = get_w() / 2 - get_cursor_x();
		y1 = get_h() / 2 - get_cursor_y();
		distance = (int)sqrt(x1 * x1 + y1 * y1);
		float s = (float)distance / (get_w() / 2);
		bclamp(s, 0, 1);  window->hsv.s = s;
		window->update_hsv();
		window->update_display();
		window->handle_event();
		return 1;
	}
	return 0;
}

int PaletteWheel::button_release_event()
{
	if( button_down ) {
		button_down = 0;
		return 1;
	}
	return 0;
}

void PaletteWheel::create_objects()
{
// Upper right
//printf("PaletteWheel::create_objects 1\n");
	float h, s, v = 1;
	float r, g, b;
	float x1, y1, x2, y2;
	float distance;
	int default_r, default_g, default_b;
	VFrame frame(0, -1, get_w(), get_h(), BC_RGBA8888, -1);
	x1 = get_w() / 2;
	y1 = get_h() / 2;
	default_r = (get_resources()->get_bg_color() & 0xff0000) >> 16;
	default_g = (get_resources()->get_bg_color() & 0xff00) >> 8;
	default_b = (get_resources()->get_bg_color() & 0xff);
//printf("PaletteWheel::create_objects 1\n");

	int highlight_r = (get_resources()->button_light & 0xff0000) >> 16;
	int highlight_g = (get_resources()->button_light & 0xff00) >> 8;
	int highlight_b = (get_resources()->button_light & 0xff);

	for( y2 = 0; y2 < get_h(); y2++ ) {
		unsigned char *row = (unsigned char*)frame.get_rows()[(int)y2];
		for( x2 = 0; x2 < get_w(); x2++ ) {
			distance = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
			if( distance > x1 ) {
				row[(int)x2 * 4] = default_r;
				row[(int)x2 * 4 + 1] = default_g;
				row[(int)x2 * 4 + 2] = default_b;
				row[(int)x2 * 4 + 3] = 0;
			}
			else
			if( distance > x1 - 1 ) {
				int r_i, g_i, b_i;
				if( get_h() - y2 < x2 ) {
					r_i = highlight_r;
					g_i = highlight_g;
					b_i = highlight_b;
				}
				else {
					r_i = 0;
					g_i = 0;
					b_i = 0;
				}

				row[(int)x2 * 4] = r_i;
				row[(int)x2 * 4 + 1] = g_i;
				row[(int)x2 * 4 + 2] = b_i;
				row[(int)x2 * 4 + 3] = 255;
			}
			else {
				h = get_angle(x1, y1, x2, y2);
				s = distance / x1;
				HSV::hsv_to_rgb(r, g, b, h, s, v);
				row[(int)x2 * 4] = (int)(r * 255);
				row[(int)x2 * 4 + 1] = (int)(g * 255);
				row[(int)x2 * 4 + 2] = (int)(b * 255);
				row[(int)x2 * 4 + 3] = 255;
			}
		}
	}
//printf("PaletteWheel::create_objects 1\n");

	draw_vframe(&frame,
		0,
		0,
		get_w(),
		get_h(),
		0,
		0,
		get_w(),
		get_h(),
		0);
//printf("PaletteWheel::create_objects 1\n");

	oldhue = window->hsv.h;
	oldsaturation = window->hsv.s;
//printf("PaletteWheel::create_objects 1\n");
	draw(oldhue, oldsaturation);
//printf("PaletteWheel::create_objects 1\n");
	flash();
//printf("PaletteWheel::create_objects 2\n");
}

float PaletteWheel::torads(float angle)
{
	return (float)angle / 360 * 2 * M_PI;
}


int PaletteWheel::draw(float hue, float saturation)
{
	int x, y, w, h;
	x = w = get_w() / 2;
	y = h = get_h() / 2;

	if( hue > 0 && hue < 90 ) {
		x = (int)(w - w * cos(torads(90 - hue)) * saturation);
		y = (int)(h - h * sin(torads(90 - hue)) * saturation);
	}
	else if( hue > 90 && hue < 180 ) {
		x = (int)(w - w * cos(torads(hue - 90)) * saturation);
		y = (int)(h + h * sin(torads(hue - 90)) * saturation);
	}
	else if( hue > 180 && hue < 270 ) {
		x = (int)(w + w * cos(torads(270 - hue)) * saturation);
		y = (int)(h + h * sin(torads(270 - hue)) * saturation);
	}
	else if( hue > 270 && hue < 360 ) {
		x = (int)(w + w * cos(torads(hue - 270)) * saturation);
		y = (int)(h - w * sin(torads(hue - 270)) * saturation);
	}
	else if( hue == 0 ) {
		x = w;
		y = (int)(h - h * saturation);
	}
	else if( hue == 90 ) {
		x = (int)(w - w * saturation);
		y = h;
	}
	else if( hue == 180 ) {
		x = w;
		y = (int)(h + h * saturation);
	}
	else if( hue == 270 ) {
		x = (int)(w + w * saturation);
		y = h;
	}

	set_inverse();
	set_color(WHITE);
	draw_circle(x - 5, y - 5, 10, 10);
	set_opaque();
	return 0;
}

int PaletteWheel::get_angle(float x1, float y1, float x2, float y2)
{
	float result = -atan2(x2 - x1, y1 - y2) * (360 / M_PI / 2);
	if( result < 0 )
		result += 360;
	return (int)result;
}

PaletteWheelValue::PaletteWheelValue(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 40, 170, BLACK)
{
	this->window = window;
	button_down = 0;
}
PaletteWheelValue::~PaletteWheelValue()
{
	delete frame;
}

void PaletteWheelValue::create_objects()
{
	frame = new VFrame(0, -1, get_w(), get_h(), BC_RGB888, -1);
	draw(window->hsv.h, window->hsv.s, window->hsv.v);
	flash();
}

int PaletteWheelValue::button_press_event()
{
//printf("PaletteWheelValue::button_press 1 %d\n", is_event_win());
	if( get_cursor_x() >= 0 && get_cursor_x() < get_w() &&
		get_cursor_y() >= 0 && get_cursor_y() < get_h() &&
		is_event_win() ) {
//printf("PaletteWheelValue::button_press 2\n");
		button_down = 1;
		cursor_motion_event();
		return 1;
	}
	return 0;
}

int PaletteWheelValue::cursor_motion_event()
{
	if( button_down && is_event_win() ) {
//printf("PaletteWheelValue::cursor_motion 1\n");
		float v = 1.0 - (float)(get_cursor_y() - 2) / (get_h() - 4);
		bclamp(v, 0, 1);  window->hsv.v = v;
		window->update_hsv();
		window->update_display();
		window->handle_event();
		return 1;
	}
	return 0;
}

int PaletteWheelValue::button_release_event()
{
	if( button_down ) {
//printf("PaletteWheelValue::button_release 1\n");
		button_down = 0;
		return 1;
	}
	return 0;
}

int PaletteWheelValue::draw(float hue, float saturation, float value)
{
	float r_f, g_f, b_f;
	int i, j, r, g, b;

	for( i = get_h() - 3; i >= 2; i-- ) {
		unsigned char *row = (unsigned char*)frame->get_rows()[i];
		HSV::hsv_to_rgb(r_f, g_f, b_f, hue, saturation,
			1.0 - (float)(i - 2) / (get_h() - 4));
		r = (int)(r_f * 255);
		g = (int)(g_f * 255);
		b = (int)(b_f * 255);
		for( j = 0; j < get_w(); j++ ) {
 			row[j * 3] = r;
 			row[j * 3 + 1] = g;
 			row[j * 3 + 2] = b;
		}
	}

	draw_3d_border(0, 0, get_w(), get_h(), 1);
	draw_vframe(frame, 2, 2, get_w() - 4, get_h() - 4,
		2, 2, get_w() - 4, get_h() - 4, 0);
	set_color(BLACK);
	draw_line(2, get_h() - 3 - (int)(value * (get_h() - 5)),
		  get_w() - 3, get_h() - 3 - (int)(value * (get_h() - 5)));
//printf("PaletteWheelValue::draw %d %f\n", __LINE__, value);

	return 0;
}

PaletteOutput::PaletteOutput(ColorWindow *window, int x, int y)
 : BC_SubWindow(x, y, 180, 30, BLACK)
{
	this->window = window;
}
PaletteOutput::~PaletteOutput()
{
}


void PaletteOutput::create_objects()
{
	draw();
	flash();
}

int PaletteOutput::handle_event()
{
	return 1;
}

int PaletteOutput::draw()
{
	int r = 255*window->rgb.r + 0.5f;
	int g = 255*window->rgb.g + 0.5f;
	int b = 255*window->rgb.b + 0.5f;
	set_color((r << 16) | (g << 8) | (b << 0));
	draw_box(2, 2, get_w() - 4, get_h() - 4);
	draw_3d_border(0, 0, get_w(), get_h(), 1);
	return 0;
}

PaletteHue::PaletteHue(ColorWindow *window, int x, int y)
 : BC_ISlider(x, y, 0, 150, 200, 0, 359, (int)(window->hsv.h), 0)
{
	this->window = window;
}
PaletteHue::~PaletteHue()
{
}

int PaletteHue::handle_event()
{
	window->hsv.h = get_value();
	window->update_hsv();
	window->handle_event();
	return 1;
}

PaletteSat::PaletteSat(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, window->hsv.s, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteSat::~PaletteSat()
{
}

int PaletteSat::handle_event()
{
	window->hsv.s = get_value();
	window->update_hsv();
	window->handle_event();
	return 1;
}


PaletteVal::PaletteVal(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, window->hsv.v, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteVal::~PaletteVal()
{
}

int PaletteVal::handle_event()
{
	window->hsv.v = get_value();
	window->update_hsv();
	window->handle_event();
	return 1;
}


PaletteRed::PaletteRed(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->rgb.r, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteRed::~PaletteRed()
{
}

int PaletteRed::handle_event()
{
	window->rgb.r = get_value();
	window->update_rgb();
	window->handle_event();
	return 1;
}

PaletteGrn::PaletteGrn(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->rgb.g, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteGrn::~PaletteGrn()
{
}

int PaletteGrn::handle_event()
{
	window->rgb.g = get_value();
	window->update_rgb();
	window->handle_event();
	return 1;
}

PaletteBlu::PaletteBlu(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->rgb.b, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteBlu::~PaletteBlu()
{
}

int PaletteBlu::handle_event()
{
	window->rgb.b = get_value();
	window->update_rgb();
	window->handle_event();
	return 1;
}

PaletteAlpha::PaletteAlpha(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->aph, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteAlpha::~PaletteAlpha()
{
}

int PaletteAlpha::handle_event()
{
	window->aph = get_value();
	window->handle_event();
	return 1;
}

PaletteLum::PaletteLum(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->yuv.y, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteLum::~PaletteLum()
{
}

int PaletteLum::handle_event()
{
	window->yuv.y = get_value();
	window->update_yuv();
	window->handle_event();
	return 1;
}

PaletteCr::PaletteCr(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->yuv.u, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteCr::~PaletteCr()
{
}

int PaletteCr::handle_event()
{
	window->yuv.u = get_value();
	window->update_yuv();
	window->handle_event();
	return 1;
}

PaletteCb::PaletteCb(ColorWindow *window, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1, window->yuv.v, 0)
{
	this->window = window;
	set_precision(0.01);
}
PaletteCb::~PaletteCb()
{
}

int PaletteCb::handle_event()
{
	window->yuv.v = get_value();
	window->update_yuv();
	window->handle_event();
	return 1;
}

void ColorWindow::update_rgb(float r, float g, float b)
{
	{ float y, u, v;
	YUV::rgb_to_yuv_f(r, g, b, y, u, v);
	u += 0.5f;  v += 0.5f;
	bclamp(y, 0, 1);    yuv.y = y;
	bclamp(u, 0, 1);    yuv.u = u;
	bclamp(v, 0, 1);    yuv.v = v; }
	{ float h, s, v;
	HSV::rgb_to_hsv(r,g,b, h,s,v);
	bclamp(h, 0, 360);  hsv.h = h;
	bclamp(s, 0, 1);    hsv.s = s;
	bclamp(v, 0, 1);    hsv.v = v; }
}

void ColorWindow::update_yuv(float y, float u, float v)
{
	u -= 0.5f;  v -= 0.5f;
	{ float r, g, b;
	YUV::yuv_to_rgb_f(r, g, b, y, u, v);
	bclamp(r, 0, 1);   rgb.r = r;
	bclamp(g, 0, 1);   rgb.g = g;
	bclamp(b, 0, 1);   rgb.b = b;
	float h, s, v;
	HSV::rgb_to_hsv(r,g,b, h, s, v);
	bclamp(h, 0, 360); hsv.h = h;
	bclamp(s, 0, 1);   hsv.s = s;
	bclamp(v, 0, 1);   hsv.v = v; }
}

void ColorWindow::update_hsv(float h, float s, float v)
{
	{ float r, g, b;
	HSV::hsv_to_rgb(r,g,b, h,s,v);
	bclamp(r, 0, 1);   rgb.r = r;
	bclamp(g, 0, 1);   rgb.g = g;
	bclamp(b, 0, 1);   rgb.b = b;
	float y, u, v;
	YUV::rgb_to_yuv_f(r, g, b, y, u, v);
	u += 0.5f;  v += 0.5f;
	bclamp(y, 0, 1);   yuv.y = y;
	bclamp(u, 0, 1);   yuv.u = u;
	bclamp(v, 0, 1);   yuv.v = v; }
}

PalletteNum::PalletteNum(ColorWindow *window, int x, int y,
	float &output, float min, float max)
 : BC_TumbleTextBox(window, output, min, max, x, y, 64)
{
	this->window = window;
	this->output = &output;
	set_increment(0.01);
	set_precision(2);
}

PalletteNum::~PalletteNum()
{
}


int PalletteHSV::handle_event()
{
	update_output();
	window->update_hsv();
	window->update_display();
	window->handle_event();
	return 1;
}

int PalletteRGB::handle_event()
{
	update_output();
	window->update_rgb();
	window->update_display();
	window->handle_event();
	return 1;
}

int PalletteYUV::handle_event()
{
	update_output();
	window->update_yuv();
	window->update_display();
	window->handle_event();
	return 1;
}

int PalletteAPH::handle_event()
{
	update_output();
	window->update_display();
	window->handle_event();
	return 1;
}

