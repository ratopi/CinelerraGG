
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

#include "bcsignals.h"
#include "file.inc"
#include "filesystem.h"
#include "ffmpeg.h"
#include "formatpopup.h"
#include "language.h"
#include "pluginserver.h"



FormatPopup::FormatPopup(ArrayList<PluginServer*> *plugindb, 
	int x, int y, int use_brender)
 : BC_ListBox(x, y, 200, 200, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1)
{
	this->plugindb = plugindb;
	this->use_brender = use_brender;
	set_tooltip(_("Change file format"));
}

void FormatPopup::create_objects()
{
	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(_(AC3_NAME)));
		format_items.append(new BC_ListBoxItem(_(AIFF_NAME)));
		format_items.append(new BC_ListBoxItem(_(AU_NAME)));
		format_items.append(new BC_ListBoxItem(_(FLAC_NAME)));
		format_items.append(new BC_ListBoxItem(_(JPEG_NAME)));
	}

	format_items.append(new BC_ListBoxItem(_(JPEG_LIST_NAME)));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(_(AVI_NAME)));
		format_items.append(new BC_ListBoxItem(_(EXR_NAME)));
		format_items.append(new BC_ListBoxItem(_(EXR_LIST_NAME)));
		format_items.append(new BC_ListBoxItem(_(WAV_NAME)));
		format_items.append(new BC_ListBoxItem(_(MOV_NAME)));
		format_items.append(new BC_ListBoxItem(_(FFMPEG_NAME)));
		format_items.append(new BC_ListBoxItem(_(AMPEG_NAME)));
		format_items.append(new BC_ListBoxItem(_(VMPEG_NAME)));
		format_items.append(new BC_ListBoxItem(_(OGG_NAME)));
		format_items.append(new BC_ListBoxItem(_(PCM_NAME)));
		format_items.append(new BC_ListBoxItem(_(PNG_NAME)));
	}

	format_items.append(new BC_ListBoxItem(_(PNG_LIST_NAME)));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(_(TGA_NAME)));
	}

	format_items.append(new BC_ListBoxItem(_(TGA_LIST_NAME)));

	if(!use_brender)
	{
		format_items.append(new BC_ListBoxItem(_(TIFF_NAME)));
	}

	format_items.append(new BC_ListBoxItem(_(TIFF_LIST_NAME)));
	update(&format_items, 0, 0, 1);
}

FormatPopup::~FormatPopup()
{
	format_items.remove_all_objects();
}

int FormatPopup::handle_event()
{
	return 0;
}


FFMPEGPopup::FFMPEGPopup(ArrayList<PluginServer*> *plugindb, int x, int y)
 : BC_ListBox(x, y, 50, 200, LISTBOX_TEXT, 0, 0, 0, 1, 0, 1)
{
	this->plugindb = plugindb;
	set_tooltip(_("Set ffmpeg file type"));
}

void FFMPEGPopup::create_objects()
{
	static const char *dirs[] = { "audio", "video", };
	for( int i=0; i<(int)(sizeof(dirs)/sizeof(dirs[0])); ++i ) {
		FileSystem fs;
		char option_path[BCTEXTLEN];
		FFMPEG::set_option_path(option_path, dirs[i]);
		fs.update(option_path);
		int total_files = fs.total_files();
		for( int j=0; j<total_files; ++j ) {
			const char *name = fs.get_entry(j)->get_name();
			const char *ext = strrchr(name,'.');
			if( !ext ) continue;
			if( !strcmp("dfl", ++ext) ) continue;
			if( !strcmp("opts", ext) ) continue;
			int k = ffmpeg_types.size();
			while( --k >= 0 && strcmp(ffmpeg_types[k]->get_text(), ext) );
			if( k >= 0 ) continue;
			ffmpeg_types.append(new BC_ListBoxItem(ext));
        	}
        }

	update(&ffmpeg_types, 0, 0, 1);
}

FFMPEGPopup::~FFMPEGPopup()
{
	ffmpeg_types.remove_all_objects();
}

int FFMPEGPopup::handle_event()
{
	return 0;
}


