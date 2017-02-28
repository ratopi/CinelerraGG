
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

#ifndef __CLIP_H__
#define __CLIP_H__

// Math macros
#undef SQR
#define SQR(x) ((x) * (x))
#undef CLIP
#define CLIP(x, y, z) ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x)))
#undef RECLIP
#define RECLIP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))
#undef CLAMP
#define CLAMP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))
#undef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#undef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#undef EQUIV
#define EQUIV(x, y) (fabs((x) - (y)) < 0.001)
#undef DISTANCE
#define DISTANCE(x1, y1, x2, y2) \
(sqrt(((x2) - (x1)) * ((x2) - (x1)) + ((y2) - (y1)) * ((y2) - (y1))))
#define TO_RAD(x) ((x) * 2 * M_PI / 360)
#define TO_DEG(x) ((x) * 360 / 2 / M_PI)

static inline int bmin(int a, int b) { return a < b ? a : b; }
static inline float bmin(float a, float b) { return a < b ? a : b; }
static inline double bmin(double a, double b) { return a < b ? a : b; }
static inline int bmax(int a, int b) { return a > b ? a : b; }
static inline float bmax(float a, float b) { return a > b ? a : b; }
static inline double bmax(double a, double b) { return a > b ? a : b; }

static inline int bclip(int &iv, int imn, int imx) {
	return iv < imn ? imn : iv > imx ? imx : iv;
}
static inline float bclip(float &fv, float fmn, float fmx) {
	return fv < fmn ? fmn : fv > fmx ? fmx : fv;
}
static inline double bclip(double &dv, double dmn, double dmx) {
	return dv < dmn ? dmn : dv > dmx ? dmx : dv;
}
static inline void bclamp(int &iv, int imn, int imx) {
	if( iv < imn ) iv = imn; else if( iv > imx ) iv = imx;
}
static inline void bclamp(float &fv, float fmn, float fmx) {
	if( fv < fmn ) fv = fmn; else if( fv > fmx ) fv = fmx;
}
static inline void bclamp(double &dv, double dmn, double dmx) {
	if( dv < dmn ) dv = dmn; else if( dv > dmx ) dv = dmx;
}
static inline void bc_rgb2yuv(float r, float g, float b, float &y, float &u, float &v)
{ //bt601, jpeg, unclipped
	y =  0.29900*r + 0.58700*g + 0.11400*b;
	u = -0.16874*r - 0.33126*g + 0.50000*b + 0.5;
	v =  0.50000*r - 0.41869*g - 0.08131*b + 0.5;
}
static inline void bc_rgb2yuv(int r, int g, int b, int &y, int &u, int &v, int max=255)
{ // clipped
	float mx = max, fr = r/mx, fg = g/mx, fb = b/mx, fy, fu, fv;
	bc_rgb2yuv(fr,fg,fb, fy,fu,fv);
	y = (int)(fy * mx + 0.5);  bclamp(y,0,max);
	u = (int)(fu * mx + 0.5);  bclamp(u,0,max);
	v = (int)(fv * mx + 0.5);  bclamp(v,0,max);
}
static inline void bc_yuv2rgb(float y, float u, float v, float &r, float &g, float &b)
{ //bt601, jpeg, unclipped
	r = y + 1.40200*v;
	g = y - 0.34414*u - 0.71414*v;
	b = y + 1.77200*u;
}
static inline void bc_yuv2rgb(int y, int u, int v, int &r, int &g, int &b, int max=255)
{ // clipped
	int ofs = (max + 1) / 2;
	float mx = max, fy = y/mx, fu = (u-ofs)/mx, fv = (v-ofs)/mx, fr, fg, fb;
	bc_yuv2rgb(fy,fu,fv, fr,fg,fb);
	r = (int)(fr * mx + 0.5);  bclamp(r,0,max);
	g = (int)(fg * mx + 0.5);  bclamp(g,0,max);
	b = (int)(fb * mx + 0.5);  bclamp(b,0,max);
}

#endif
