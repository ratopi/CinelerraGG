
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

#ifndef PLUGINSERVER_INC
#define PLUGINSERVER_INC

class PluginObj;
class PluginServer;

#define PLUGIN_LADSPA_ID 0
#define PLUGIN_FFMPEG_ID 1
#define PLUGIN_IDS 2

#define PLUGIN_TYPE_UNKNOWN -1
#define PLUGIN_TYPE_EXECUTABLE 0
#define PLUGIN_TYPE_BUILTIN 1
#define PLUGIN_TYPE_LADSPA 2
#define PLUGIN_TYPE_FFMPEG 3
#define PLUGIN_TYPES 4

#endif
