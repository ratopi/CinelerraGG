
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

#ifndef COLORPICKER_H
#define COLORPICKER_H

#include "bcdialog.h"
#include "clip.h"
#include "condition.inc"
#include "guicast.h"
#include "mutex.inc"
#include "thread.h"
#include "vframe.inc"

class ColorWindow;
class PaletteWheel;
class PaletteWheelValue;
class PaletteOutput;
class PaletteHue;
class PaletteSat;
class PaletteVal;
class PaletteRed;
class PaletteGrn;
class PaletteBlu;
class PaletteLum;
class PaletteCr;
class PaletteCb;
class PaletteAlpha;
class PalletteHSV;
class PalletteRGB;
class PalletteYUV;
class PalletteAPH;

class ColorThread : public BC_DialogThread
{
public:
	ColorThread(int do_alpha = 0, const char *title = 0);
	~ColorThread();

	void start_window(int output, int alpha, int do_okcancel=0);
	virtual int handle_new_color(int output, int alpha);
	void update_gui(int output, int alpha);
	BC_Window* new_gui();

	int output, alpha;
	int do_alpha, do_okcancel;
	const char *title;
};

class ColorWindow : public BC_Window
{
public:
	ColorWindow(ColorThread *thread, int x, int y, int w, int h, const char *title);
	~ColorWindow();

	void create_objects();
	void change_values();
	int close_event();
	void update_display();
	void update_rgb();
	void update_hsv();
	void update_yuv();
	int handle_event();

	struct { float r, g, b; } rgb;
	struct { float y, u, v; } yuv;
	struct { float h, s, v; } hsv;
	float aph;
	void update_rgb(float r, float g, float b);
	void update_hsv(float h, float s, float v);
	void update_yuv(float y, float u, float v);

	ColorThread *thread;

	PaletteWheel *wheel;
	PaletteWheelValue *wheel_value;
	PaletteOutput *output;
	PaletteHue *hue;
	PaletteSat *sat;
	PaletteVal *val;
	PaletteRed *red;
	PaletteGrn *grn;
	PaletteBlu *blu;
	PaletteLum *lum;
	PaletteCr  *c_r;
	PaletteCb  *c_b;
	PaletteAlpha *alpha;

	PalletteHSV *hsv_h, *hsv_s, *hsv_v;
	PalletteRGB *rgb_r, *rgb_g, *rgb_b;
	PalletteYUV *yuv_y, *yuv_u, *yuv_v;
	PalletteAPH *aph_a;

	VFrame *value_bitmap;
};


class PaletteWheel : public BC_SubWindow
{
public:
	PaletteWheel(ColorWindow *window, int x, int y);
	~PaletteWheel();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();

	void create_objects();
	int draw(float hue, float saturation);
	int get_angle(float x1, float y1, float x2, float y2);
	float torads(float angle);
	ColorWindow *window;
	float oldhue;
	float oldsaturation;
	int button_down;
};

class PaletteWheelValue : public BC_SubWindow
{
public:
	PaletteWheelValue(ColorWindow *window, int x, int y);
	~PaletteWheelValue();
	void create_objects();
	int button_press_event();
	int cursor_motion_event();
	int button_release_event();
	int draw(float hue, float saturation, float value);
	ColorWindow *window;
	int button_down;
// Gradient
	VFrame *frame;
};

class PaletteOutput : public BC_SubWindow
{
public:
	PaletteOutput(ColorWindow *window, int x, int y);
	~PaletteOutput();
	void create_objects();
	int handle_event();
	int draw();
	ColorWindow *window;
};

class PaletteHue : public BC_ISlider
{
public:
	PaletteHue(ColorWindow *window, int x, int y);
	~PaletteHue();
	int handle_event();
	ColorWindow *window;
};

class PaletteSat : public BC_FSlider
{
public:
	PaletteSat(ColorWindow *window, int x, int y);
	~PaletteSat();
	int handle_event();
	ColorWindow *window;
};

class PaletteVal : public BC_FSlider
{
public:
	PaletteVal(ColorWindow *window, int x, int y);
	~PaletteVal();
	int handle_event();
	ColorWindow *window;
};

class PaletteRed : public BC_FSlider
{
public:
	PaletteRed(ColorWindow *window, int x, int y);
	~PaletteRed();
	int handle_event();
	ColorWindow *window;
};

class PaletteGrn : public BC_FSlider
{
public:
	PaletteGrn(ColorWindow *window, int x, int y);
	~PaletteGrn();
	int handle_event();
	ColorWindow *window;
};

class PaletteBlu : public BC_FSlider
{
public:
	PaletteBlu(ColorWindow *window, int x, int y);
	~PaletteBlu();
	int handle_event();
	ColorWindow *window;
};

class PaletteAlpha : public BC_FSlider
{
public:
	PaletteAlpha(ColorWindow *window, int x, int y);
	~PaletteAlpha();
	int handle_event();
	ColorWindow *window;
};

class PaletteLum : public BC_FSlider
{
public:
	PaletteLum(ColorWindow *window, int x, int y);
	~PaletteLum();
	int handle_event();
	ColorWindow *window;
};

class PaletteCr : public BC_FSlider
{
public:
	PaletteCr(ColorWindow *window, int x, int y);
	~PaletteCr();
	int handle_event();
	ColorWindow *window;
};

class PaletteCb : public BC_FSlider
{
public:
	PaletteCb(ColorWindow *window, int x, int y);
	~PaletteCb();
	int handle_event();
	ColorWindow *window;
};

class PalletteNum : public BC_TumbleTextBox
{
public:
	ColorWindow *window;
	float *output;

	PalletteNum(ColorWindow *window, int x, int y,
			float &output, float min, float max);
	~PalletteNum();
	void update_output() { *output = atof(get_text()); }
	static int calculate_h() { return BC_Tumbler::calculate_h(); }
};

class PalletteRGB : public PalletteNum
{
public:
	PalletteRGB(ColorWindow *window, int x, int y,
			float &output, float min, float max)
	 : PalletteNum(window, x, y, output, min, max) {}
	int handle_event();
};

class PalletteYUV : public PalletteNum
{
public:
	PalletteYUV(ColorWindow *window, int x, int y,
			float &output, float min, float max)
	 : PalletteNum(window, x, y, output, min, max) {}
	int handle_event();
};

class PalletteHSV : public PalletteNum
{
public:
	PalletteHSV(ColorWindow *window, int x, int y,
			float &output, float min, float max)
	 : PalletteNum(window, x, y, output, min, max) {}
	int handle_event();
};

class PalletteAPH : public PalletteNum
{
public:
	PalletteAPH(ColorWindow *window, int x, int y,
			float &output, float min, float max)
	 : PalletteNum(window, x, y, output, min, max) {}
	int handle_event();
};

#endif
