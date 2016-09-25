
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

#ifndef OVERLAYFRAME_INC
#define OVERLAYFRAME_INC

// Modes

/*
source
x = porter duff operator
g = good guy
a = android
* = hv 4.6.1

cin 5.1 overlayframe TRANSFER_MODEs
x	DST [Da, Dc]
x	DST_ATOP [Sa, Sc * (1 - Da) + Dc * Sa]
x	DST_IN  [Da * Sa, Dc * Sa]
x	DST_OUT [Da * (1 - Sa), Dc * (1 - Sa)]
x	DST_OVER [Sa * (1 - Da) + Da, Sc * (1 - Da) + Dc]
x	SRC [Sa, Sc]
x	SRC_ATOP [Da, Sc * Da + Dc * (1 - Sa)]
x	SRC_IN  [Sa * Da, Sc * Da]
x	SRC_OUT [Sa * (1 - Da), Sc * (1 - Da)]
x	SRC_OVER [Sa + Da * (1 - Sa), Sc + (1 - Sa) * Dc]
g	OR [Sa + Da - Sa * Da, Sc + Dc - Sc * Dc]
a	XOR     [Sa * (1 - Da) + Da * (1 - Sa), Sc * (1 - Da) + Dc * (1 ­ Sa)]
*	NORMAL [Sa + Da * (1 - Sa), Sc * Sa + Dc * (1 ­ Sa)])
*	ADDITION [(Sa + Da), (Sc + Dc)]
*	SUBTRACT [(Sa - Da), (Sc ­ Dc)]
*	MULTIPLY [(Sa * Da), Sc * Dc]
*	DIVIDE  [(Sa / Da), (Sc / Dc)]
*	REPLACE [Sa, Sc] (fade = 1)
*	MAX     [max(Sa, Da), MAX(Sc, Dc)]
*	MIN [min(Sa, Da), MIN(Sc, Dc)]
g	AVERAGE [(Sa + Da) * 0.5, (Sc + Dc) * 0.5]
a	DARKEN [Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) + min(Sc, Dc)]
a	LIGHTEN [Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) + max(Sc, Dc)]

https://developer.android.com/reference/android/graphics/PorterDuff.Mode.html
	ADD Saturate(S + D)
	CLEAR [0, 0]  
	DARKEN [Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) + min(Sc, Dc)]  
	DST [Da, Dc]  
	DST_ATOP [Sa, Sa * Dc + Sc * (1 - Da)]  
	DST_IN [Sa * Da, Sa * Dc]  
	DST_OUT [Da * (1 - Sa), Dc * (1 - Sa)]  
	DST_OVER [Sa + (1 - Sa)*Da, Rc = Dc + (1 - Da)*Sc]  
	LIGHTEN [Sa + Da - Sa*Da, Sc*(1 - Da) + Dc*(1 - Sa) + max(Sc, Dc)]  
	MULTIPLY [Sa * Da, Sc * Dc]  
	OVERLAY (defined, but unspecified in android)
	SCREEN [Sa + Da - Sa * Da, Sc + Dc - Sc * Dc]  
	SRC [Sa, Sc]  
	SRC_ATOP [Da, Sc * Da + (1 - Sa) * Dc]  
	SRC_IN [Sa * Da, Sc * Da]  
	SRC_OUT [Sa * (1 - Da), Sc * (1 - Da)]  
	SRC_OVER [Sa + (1 - Sa)*Da, Rc = Sc + (1 - Sa)*Dc]  
	XOR [Sa + Da - 2 * Sa * Da, Sc * (1 - Da) + (1 - Sa) * Dc]  

https://docs.gimp.org/2.6/en/gimp-concepts-layer-modes.html
	Normal          [M]
	Dissolve        [I + dots]
	Multiply        [M * I]
	Divide          [I / M]
	Screen          [1 - (1-M)*(1-I)]
	Overlay         [I * (I + 2*M*(1-I))]
	Dodge           [I / (1-M)]
	Burn            [1 - (1-I) / M]
	Hard light      [M>0.5 ? (1 - (1-2*(M-0.5))*(1 - I) : 2*M*I]
	Soft light      [((1-I)*M + Screen()) * I]
	Grain extract   [I - M + 0.5]
	Grain merge     [I + M - 0.5]
	Difference      [abs(I-M)]
	Addition        [min((I+M),1.0)]
	Subtract        [max((I-M),0.0)]
	Darken only     [min(M,I)]
	Lighten only    [max(M,I)]
	Hue             [hue(M) + sat(I)+val(I)]
	Saturation      [sat(M) + hue(I)+val(I)]
	Color           [hue(M)+sat(M) + val(I)]
	Value           [val(M) + sat(I)+hue(I)]

*/

#define TRANSFER_TYPES 23

#define TRANSFER_NORMAL		0
#define TRANSFER_ADDITION	1
#define TRANSFER_SUBTRACT	2
#define TRANSFER_MULTIPLY	3
#define TRANSFER_DIVIDE		4
#define TRANSFER_REPLACE	5
#define TRANSFER_MAX		6
#define TRANSFER_MIN		7
#define TRANSFER_AVERAGE	8
#define TRANSFER_DARKEN		9
#define TRANSFER_LIGHTEN	10
#define TRANSFER_DST		11
#define TRANSFER_DST_ATOP	12
#define TRANSFER_DST_IN		13
#define TRANSFER_DST_OUT	14
#define TRANSFER_DST_OVER	15
#define TRANSFER_SRC		16
#define TRANSFER_SRC_ATOP	17
#define TRANSFER_SRC_IN		18
#define TRANSFER_SRC_OUT	19
#define TRANSFER_SRC_OVER	20
#define TRANSFER_OR		21
#define TRANSFER_XOR		22

// Interpolation types


#define NEAREST_NEIGHBOR 0        // Nearest for enlargement and reduction
#define CUBIC_CUBIC      1        // Bicubic enlargement and reduction
#define CUBIC_LINEAR     2        // Bicubic enlargement and linear reduction
#define LINEAR_LINEAR    3        // Not explicitely called but used for user
#define LANCZOS_LANCZOS  4        // Just because we can

class OverlayFrame;

#endif
