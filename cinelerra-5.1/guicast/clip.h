
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

#ifndef CLIP_H
#define CLIP_H

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

#endif
