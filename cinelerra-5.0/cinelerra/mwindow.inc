
/*
 * CINELERRA
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

#ifndef MWINDOW_INC
#define MWINDOW_INC

#define PROGRAM_NAME "Cinelerra"
#define CINELERRA_VERSION "5.0"
#define COPYRIGHT_DATE 2015
#define DEFAULT_THEME "S.U.V."
#define CONFIG_FILE "Cinelerra_rc"
#define PRESETS_FILE "Cinelerra_presets"
#define PICTURE_FILE "Cinelerra_picture"
#define PLUGIN_FILE "Cinelerra_plugins"
#define LADSPA_FILE "ladspa_plugins."
#define PLUGIN_FILE_VERSION 3

// Behavior of region selections
#define SELECTION_SAMPLES 0
#define SELECTION_EDIT 1

// min milliseconds between tweek undos to create new undo entries
#define UNDO_SPAN 1000

// Make tweeks span keyframes if a region is selected
#define USE_KEYFRAME_SPANNING

// Index of vwindow that always exists
#define DEFAULT_VWINDOW 0

class MWindow;

#if 0
// for PROGRAM_NAME xlat strings
N_("Cinelerra: Attach Effect")
N_("Cinelerra: Audio compression")
N_("Cinelerra: Audio Compression")
N_("Cinelerra: Camera")
N_("Cinelerra: Change Effect")
N_("Cinelerra: Channel Info")
N_("Cinelerra: ChanSearch")
N_("Cinelerra: Clip Info")
N_("Cinelerra: Color")
N_("Cinelerra: Compositor")
N_("Cinelerra: Confirm")
N_("Cinelerra: Confirm Quit")
N_("Cinelerra: Crop")
N_("Cinelerra: DbWindow")
N_("Cinelerra: Delete All Indexes")
N_("Cinelerra: Edit length")
N_("Cinelerra: Error")
N_("Cinelerra: Errors")
N_("Cinelerra: File Exists")
N_("Cinelerra: File Format")
N_("Cinelerra: Levels")
N_("Cinelerra: Load")
N_("Cinelerra: Loading")
N_("Cinelerra: Locate file")
N_("Cinelerra: Mask")
N_("Cinelerra: New folder")
N_("Cinelerra: Overlays")
N_("Cinelerra: Path")
N_("Cinelerra: Preferences")
N_("Cinelerra: Program")
N_("Cinelerra: Projector")
N_("Cinelerra: Question")
N_("Cinelerra: RemoteWindow")
N_("Cinelerra: Remove assets")
N_("Cinelerra: Resize Track")
N_("Cinelerra: Ruler")
N_("Cinelerra: %s")
N_("Cinelerra: Save")
N_("Cinelerra: Set edit title")
N_("Cinelerra: Set Format")
N_("Cinelerra: %s Keyframe")
N_("Cinelerra: Subtitle")
N_("Cinelerra: Tip of the day")
N_("Cinelerra: Transition length")
N_("Cinelerra: Video Compression")
N_("Cinelerra: Viewer")
N_("Cinelerra: Warning")
N_("Cinelerra: New Project")
N_("Cinelerra: New Project")

N_("Cinelerra: CD Ripper")
N_("Cinelerra: Normalize")
N_("Cinelerra: Resample")
N_("Cinelerra: Time stretch")
#endif

#endif
