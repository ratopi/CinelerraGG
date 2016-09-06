
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

#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "svg.h"
#include "svgwin.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>


REGISTER_PLUGIN(SvgMain)

SvgConfig::SvgConfig()
{
	out_x = 0;
	out_y = 0;
	strcpy(svg_file, "");
	ms_time = 0;
}

int SvgConfig::equivalent(SvgConfig &that)
{
	// out_x/out_y always used by overlayer
	return !strcmp(svg_file, that.svg_file) &&
		ms_time == that.ms_time;
}

void SvgConfig::copy_from(SvgConfig &that)
{
	out_x = that.out_x;
	out_y = that.out_y;
	strcpy(svg_file, that.svg_file);
	ms_time = that.ms_time;
}

void SvgConfig::interpolate(SvgConfig &prev, SvgConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->out_x = prev.out_x * prev_scale + next.out_x * next_scale;
	this->out_y = prev.out_y * prev_scale + next.out_y * next_scale;
	strcpy(this->svg_file, prev.svg_file);
	this->ms_time = prev.ms_time;
}


SvgMain::SvgMain(PluginServer *server)
 : PluginVClient(server)
{
	ofrm = 0;
	overlayer = 0;
	need_reconfigure = 1;
}

SvgMain::~SvgMain()
{
	delete ofrm;
	delete overlayer;
}

const char* SvgMain::plugin_title() { return _("SVG via Inkscape"); }
int SvgMain::is_realtime() { return 1; }
int SvgMain::is_synthesis() { return 1; }


LOAD_CONFIGURATION_MACRO(SvgMain, SvgConfig)

void SvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->get_data(), MESSAGESIZE);

	output.tag.set_title("SVG");
	output.tag.set_property("OUT_X", config.out_x);
	output.tag.set_property("OUT_Y", config.out_y);
	output.tag.set_property("SVG_FILE", config.svg_file);
	output.tag.set_property("MS_TIME", config.ms_time);
	output.append_tag();
	output.tag.set_title("/SVG");
	output.append_tag();

	output.terminate_string();
}

void SvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	const char *data = keyframe->get_data();
	input.set_shared_input((char*)data, strlen(data));
	int result = 0;

	while( !(result = input.read_tag()) ) {
		if(input.tag.title_is("SVG")) {
			config.out_x =	input.tag.get_property("OUT_X", config.out_x);
			config.out_y =	input.tag.get_property("OUT_Y", config.out_y);
			input.tag.get_property("SVG_FILE", config.svg_file);
			config.ms_time = input.tag.get_property("MS_TIME", config.ms_time);
		}
	}
}


int SvgMain::process_realtime(VFrame *input, VFrame *output)
{
	if( input != output )
		output->copy_from(input);

	need_reconfigure |= load_configuration();
	if( need_reconfigure ) {
		need_reconfigure = 0;
		if( config.svg_file[0] == 0 ) return 0;
		delete ofrm;  ofrm = 0;
		char filename_png[1024];
		strcpy(filename_png, config.svg_file);
		strncat(filename_png, ".png", sizeof(filename_png));
		struct stat st_png;
		int64_t ms_time = stat(filename_png, &st_png) ? 0 :
			st_png.st_mtim.tv_sec*1000 + st_png.st_mtim.tv_nsec/1000000;
		int fd = ms_time < config.ms_time ? -1 : open(filename_png, O_RDWR);
		if( fd < 0 ) { // file does not exist, export it
			char command[1024];
			sprintf(command,
				"inkscape --without-gui --export-background=0x000000 "
				"--export-background-opacity=0 %s --export-png=%s",
				config.svg_file, filename_png);
			printf(_("Running command %s\n"), command);
			system(command);
			// in order for lockf to work it has to be open for writing
			fd = open(filename_png, O_RDWR);
			if( fd < 0 )
				printf(_("Export of %s to %s failed\n"), config.svg_file, filename_png);
		}
		if( fd >= 0 ) {
			struct stat st_png;
			fstat(fd, &st_png);
			unsigned char *png_buffer = (unsigned char *)
				mmap (NULL, st_png.st_size, PROT_READ, MAP_SHARED, fd, 0); 
			if( png_buffer != MAP_FAILED ) {
				if( png_buffer[0] == 0x89 && png_buffer[1] == 0x50 &&
				    png_buffer[2] == 0x4e && png_buffer[3] == 0x47 ) {
					ofrm = new VFramePng(png_buffer, st_png.st_size, 1., 1.);
					if( ofrm->get_color_model() != output->get_color_model() ) {
						VFrame *vfrm = new VFrame(ofrm->get_w(), ofrm->get_h(),
							output->get_color_model());
						vfrm->transfer_from(ofrm);
						delete ofrm;  ofrm = vfrm;
					}
				}
				else
					printf (_("The file %s that was generated from %s is not in PNG format."
						  " Try to delete all *.png files.\n"), filename_png, config.svg_file);	
				munmap(png_buffer, st_png.st_size);
			}
			else
				printf(_("Access mmap to %s as %s failed.\n"), config.svg_file, filename_png);
			close(fd);
		}
	}
	if( ofrm ) {
		if(!overlayer) overlayer = new OverlayFrame(smp + 1);
		overlayer->overlay(output, ofrm,
			 0, 0, ofrm->get_w(), ofrm->get_h(),
			config.out_x, config.out_y, 
			config.out_x + ofrm->get_w(),
			config.out_y + ofrm->get_h(),
			1, TRANSFER_NORMAL,
			get_interpolation_type());
	}
	return 0;
}


NEW_WINDOW_MACRO(SvgMain, SvgWin)

void SvgMain::update_gui()
{
	if(thread) {
		load_configuration();
		SvgWin *window = (SvgWin*)thread->window;
		window->update_gui(config);
	}
}

