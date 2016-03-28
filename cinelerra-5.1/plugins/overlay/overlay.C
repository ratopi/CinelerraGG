
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "edlsession.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "overlayframe.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <string.h>
#include <stdint.h>


class Overlay;
class OverlayWindow;


class OverlayConfig
{
public:
	OverlayConfig();

	static const char* mode_to_text(int mode);
	int mode;

	static const char* direction_to_text(int direction);
	int direction;
	enum
	{
		BOTTOM_FIRST,
		TOP_FIRST
	};

	static const char* output_to_text(int output_layer);
	int output_layer;
	enum
	{
		TOP,
		BOTTOM
	};
};

class OverlayMode : public BC_PopupMenu
{
public:
	OverlayMode(Overlay *plugin,
		int x,
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};

class OverlayDirection : public BC_PopupMenu
{
public:
	OverlayDirection(Overlay *plugin,
		int x,
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};

class OverlayOutput : public BC_PopupMenu
{
public:
	OverlayOutput(Overlay *plugin,
		int x,
		int y);
	void create_objects();
	int handle_event();
	Overlay *plugin;
};


class OverlayWindow : public PluginClientWindow
{
public:
	OverlayWindow(Overlay *plugin);
	~OverlayWindow();

	void create_objects();


	Overlay *plugin;
	OverlayMode *mode;
	OverlayDirection *direction;
	OverlayOutput *output;
};

class Overlay : public PluginVClient
{
public:
	Overlay(PluginServer *server);
	~Overlay();


	PLUGIN_CLASS_MEMBERS(OverlayConfig);

	int process_buffer(VFrame **frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	int is_multichannel();
	int is_synthesis();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	OverlayFrame *overlayer;
	VFrame *temp;
	int current_layer;
	int output_layer;
	int input_layer;
};

OverlayConfig::OverlayConfig()
{
	mode = TRANSFER_NORMAL;
	direction = OverlayConfig::BOTTOM_FIRST;
	output_layer = OverlayConfig::TOP;
}

const char* OverlayConfig::mode_to_text(int mode)
{
	switch(mode) {
	case TRANSFER_NORMAL:		return _("Normal");
	case TRANSFER_ADDITION:		return _("Addition");
	case TRANSFER_SUBTRACT:		return _("Subtract");
	case TRANSFER_MULTIPLY:		return _("Multiply");
	case TRANSFER_DIVIDE:		return _("Divide");
	case TRANSFER_REPLACE:		return _("Replace");
	case TRANSFER_MAX:		return _("Max");
	case TRANSFER_MIN:		return _("Min");
	case TRANSFER_AVERAGE:		return _("Average");
	case TRANSFER_DARKEN:		return _("Darken");
	case TRANSFER_LIGHTEN:		return _("Lighten");
	case TRANSFER_DST:		return _("Dst");
	case TRANSFER_DST_ATOP:		return _("DstAtop");
	case TRANSFER_DST_IN:		return _("DstIn");
	case TRANSFER_DST_OUT:		return _("DstOut");
	case TRANSFER_DST_OVER:		return _("DstOver");
	case TRANSFER_SRC:		return _("Src");
	case TRANSFER_SRC_ATOP:		return _("SrcAtop");
	case TRANSFER_SRC_IN:		return _("SrcIn");
	case TRANSFER_SRC_OUT:		return _("SrcOut");
	case TRANSFER_SRC_OVER:		return _("SrcOver");
	case TRANSFER_OR:		return _("Or");
	case TRANSFER_XOR:		return _("Xor");
	default:			break;
	}
	return _("Normal");
}

const char* OverlayConfig::direction_to_text(int direction)
{
	switch(direction)
	{
		case OverlayConfig::BOTTOM_FIRST: return _("Bottom first");
		case OverlayConfig::TOP_FIRST:    return _("Top first");
	}
	return "";
}

const char* OverlayConfig::output_to_text(int output_layer)
{
	switch(output_layer)
	{
		case OverlayConfig::TOP:    return _("Top");
		case OverlayConfig::BOTTOM: return _("Bottom");
	}
	return "";
}









OverlayWindow::OverlayWindow(Overlay *plugin)
 : PluginClientWindow(plugin,
	300,
	160,
	300,
	160,
	0)
{
	this->plugin = plugin;
}

OverlayWindow::~OverlayWindow()
{
}

void OverlayWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	add_subwindow(mode = new OverlayMode(plugin,
		x + title->get_w() + 5,
		y));
	mode->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Layer order:")));
	add_subwindow(direction = new OverlayDirection(plugin,
		x + title->get_w() + 5,
		y));
	direction->create_objects();

	y += 30;
	add_subwindow(title = new BC_Title(x, y, _("Output layer:")));
	add_subwindow(output = new OverlayOutput(plugin,
		x + title->get_w() + 5,
		y));
	output->create_objects();

	show_window();
	flush();
}







OverlayMode::OverlayMode(Overlay *plugin, int x, int y)
 : BC_PopupMenu(x, y, 150,
	OverlayConfig::mode_to_text(plugin->config.mode), 1)
{
	this->plugin = plugin;
}

void OverlayMode::create_objects()
{
	for(int i = 0; i < TRANSFER_TYPES; i++)
		add_item(new BC_MenuItem(OverlayConfig::mode_to_text(i)));
}

int OverlayMode::handle_event()
{
	char *text = get_text();

	for(int i = 0; i < TRANSFER_TYPES; i++)
	{
		if(!strcmp(text, OverlayConfig::mode_to_text(i)))
		{
			plugin->config.mode = i;
			break;
		}
	}

	plugin->send_configure_change();
	return 1;
}


OverlayDirection::OverlayDirection(Overlay *plugin,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	150,
	OverlayConfig::direction_to_text(plugin->config.direction),
	1)
{
	this->plugin = plugin;
}

void OverlayDirection::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP_FIRST)));
	add_item(new BC_MenuItem(
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM_FIRST)));
}

int OverlayDirection::handle_event()
{
	char *text = get_text();

	if(!strcmp(text,
		OverlayConfig::direction_to_text(
			OverlayConfig::TOP_FIRST)))
		plugin->config.direction = OverlayConfig::TOP_FIRST;
	else
	if(!strcmp(text,
		OverlayConfig::direction_to_text(
			OverlayConfig::BOTTOM_FIRST)))
		plugin->config.direction = OverlayConfig::BOTTOM_FIRST;

	plugin->send_configure_change();
	return 1;
}


OverlayOutput::OverlayOutput(Overlay *plugin,
	int x,
	int y)
 : BC_PopupMenu(x,
 	y,
	100,
	OverlayConfig::output_to_text(plugin->config.output_layer),
	1)
{
	this->plugin = plugin;
}

void OverlayOutput::create_objects()
{
	add_item(new BC_MenuItem(
		OverlayConfig::output_to_text(
			OverlayConfig::TOP)));
	add_item(new BC_MenuItem(
		OverlayConfig::output_to_text(
			OverlayConfig::BOTTOM)));
}

int OverlayOutput::handle_event()
{
	char *text = get_text();

	if(!strcmp(text,
		OverlayConfig::output_to_text(
			OverlayConfig::TOP)))
		plugin->config.output_layer = OverlayConfig::TOP;
	else
	if(!strcmp(text,
		OverlayConfig::output_to_text(
			OverlayConfig::BOTTOM)))
		plugin->config.output_layer = OverlayConfig::BOTTOM;

	plugin->send_configure_change();
	return 1;
}




















REGISTER_PLUGIN(Overlay)






Overlay::Overlay(PluginServer *server)
 : PluginVClient(server)
{

	overlayer = 0;
	temp = 0;
}


Overlay::~Overlay()
{

	if(overlayer) delete overlayer;
	if(temp) delete temp;
}



int Overlay::process_buffer(VFrame **frame,
	int64_t start_position,
	double frame_rate)
{
	load_configuration();

	EDLSession* session = get_edlsession();
	int interpolation_type = session ? session->interpolation_type : NEAREST_NEIGHBOR;

	int step = config.direction == OverlayConfig::BOTTOM_FIRST ?  -1 : 1;
	int layers = get_total_buffers();
	input_layer = config.direction == OverlayConfig::BOTTOM_FIRST ? layers-1 : 0;
	output_layer = config.output_layer == OverlayConfig::TOP ?  0 : layers-1;
	VFrame *output = frame[output_layer];

	current_layer = input_layer;
	read_frame(output, current_layer,  // Direct copy the first layer
			 start_position, frame_rate, get_use_opengl());

	if( --layers > 0 ) {	// need 2 layers to do overlay
		if( !temp )
			temp = new VFrame(0, -1, frame[0]->get_w(), frame[0]->get_h(),
					frame[0]->get_color_model(), -1);

		while( --layers >= 0 ) {
			current_layer += step;
			read_frame(temp, current_layer,
				 start_position, frame_rate, get_use_opengl());

			if(get_use_opengl()) {
				run_opengl();
				continue;
			}

			if(!overlayer)
				overlayer = new OverlayFrame(get_project_smp() + 1);
			
			overlayer->overlay(output, temp,
				0, 0, output->get_w(), output->get_h(),
				0, 0, output->get_w(), output->get_h(),
				1, config.mode, interpolation_type);
		}
	}

	return 0;
}

int Overlay::handle_opengl()
{
#ifdef HAVE_GL
	static const char *get_pixels_frag =
		"uniform sampler2D src_tex;\n"
		"uniform sampler2D dst_tex;\n"
		"uniform vec2 dst_tex_dimensions;\n"
		"uniform vec3 chroma_offset;\n"
		"void main()\n"
		"{\n"
		"	vec4 dst_color = texture2D(dst_tex, gl_FragCoord.xy / dst_tex_dimensions);\n"
		"	vec4 src_color = texture2D(src_tex, gl_TexCoord[0].st);\n"
		"	src_color.rgb -= chroma_offset;\n"
		"	dst_color.rgb -= chroma_offset;\n";

	static const char *put_pixels_frag =
		"	result.rgb += chroma_offset;\n"
		"	gl_FragColor = result;\n"
		"}\n";


// NORMAL
static const char *blend_normal_frag =
	"	vec4 result = mix(src_color, src_color, src_color.a);\n";

// ADDITION
static const char *blend_add_frag =
	"	vec4 result = dst_color + src_color;\n"
	"	result = clamp(result, 0.0, 1.0);\n";

// SUBTRACT
static const char *blend_subtract_frag =
	"	vec4 result = dst_color - src_color;\n"
	"	result = clamp(result, 0.0, 1.0);\n";

// MULTIPLY
static const char *blend_multiply_frag =
	"	vec4 result = dst_color * src_color;\n";

// DIVIDE
static const char *blend_divide_frag =
	"	vec4 result = dst_color / src_color;\n"
	"	if(src_color.r == 0.) result.r = 1.0;\n"
	"	if(src_color.g == 0.) result.g = 1.0;\n"
	"	if(src_color.b == 0.) result.b = 1.0;\n"
	"	if(src_color.a == 0.) result.a = 1.0;\n"
	"	result = clamp(result, 0.0, 1.0);\n";

// MAX
static const char *blend_max_frag =
	"	vec4 result = max(src_color, dst_color);\n";

// MIN
static const char *blend_min_frag =
	"	vec4 result = min(src_color, dst_color);\n";

// AVERAGE
static const char *blend_average_frag =
	"	vec4 result = (src_color + dst_color) * 0.5;\n";

// DARKEN
static const char *blend_darken_frag =
	"	vec4 result = vec4(src_color.rgb * (1.0 - dst_color.a) +"
			" dst_color.rgb * (1.0 - src_color.a) +"
			" min(src_color.rgb, dst_color.rgb), "
			" src_color.a + dst_color.a - src_color.a * dst_color.a);\n"
	"	result = clamp(result, 0.0, 1.0);\n";

// LIGHTEN
static const char *blend_lighten_frag =
	"	vec4 result = vec4(src_color.rgb * (1.0 - dst_color.a) +"
			" dst_color.rgb * (1.0 - src_color.a) +"
			" max(src_color.rgb, dst_color.rgb), "
			" src_color.a + dst_color.a - src_color.a * dst_color.a);\n"
	"	result = clamp(result, 0.0, 1.0);\n";

// DST
static const char *blend_dst_frag =
	"	vec4 result = dst_color;\n";

// DST_ATOP
static const char *blend_dst_atop_frag =
	"	vec4 result = vec4(src_color.rgb * dst_color.a + "
			"(1.0 - src_color.a) * dst_color.rgb, dst_color.a);\n";

// DST_IN
static const char *blend_dst_in_frag =
	"	vec4 result = src_color * dst_color.a;\n";

// DST_OUT
static const char *blend_dst_out_frag =
	"	vec4 result = src_color * (1.0 - dst_color.a);\n";

// DST_OVER
static const char *blend_dst_over_frag =
	"	vec4 result = vec4(src_color.rgb + (1.0 - src_color.a) * dst_color.rgb, "
			" dst_color.a + src_color.a - dst_color.a * src_color.a);\n";

// SRC
static const char *blend_src_frag =
	"	vec4 result = src_color;\n";

// SRC_ATOP
static const char *blend_src_atop_frag =
	"	vec4 result = vec4(dst_color.rgb * src_color.a + "
			"src_color.rgb * (1.0 - dst_color.a), src_color.a);\n";

// SRC_IN
static const char *blend_src_in_frag =
	"	vec4 result = dst_color * src_color.a;\n";

// SRC_OUT
static const char *blend_src_out_frag =
	"	vec4 result = dst_color * (1.0 - src_color.a);\n";

// SRC_OVER
static const char *blend_src_over_frag =
	"	vec4 result = vec4(dst_color.rgb + (1.0 - dst_color.a) * src_color.rgb, "
				"dst_color.a + src_color.a - dst_color.a * src_color.a);\n";

// OR
static const char *blend_or_frag =
	"	vec4 result = src_color + dst_color - src_color * dst_color;\n";

// XOR
static const char *blend_xor_frag =
	"	vec4 result = vec4(dst_color.rgb * (1.0 - src_color.a) + "
			"(1.0 - dst_color.a) * src_color.rgb, "
			"dst_color.a + src_color.a - 2.0 * dst_color.a * src_color.a);\n";

static const char * const overlay_shaders[TRANSFER_TYPES] = {
		blend_normal_frag,	// TRANSFER_NORMAL
		blend_add_frag,		// TRANSFER_ADDITION
		blend_subtract_frag,	// TRANSFER_SUBTRACT
		blend_multiply_frag,	// TRANSFER_MULTIPLY
		blend_divide_frag,	// TRANSFER_DIVIDE
		blend_src_frag,		// TRANSFER_REPLACE
		blend_max_frag,		// TRANSFER_MAX
		blend_min_frag,		// TRANSFER_MIN
		blend_average_frag,	// TRANSFER_AVERAGE
		blend_darken_frag,	// TRANSFER_DARKEN
		blend_lighten_frag,	// TRANSFER_LIGHTEN
		blend_dst_frag,		// TRANSFER_DST
		blend_dst_atop_frag,	// TRANSFER_DST_ATOP
		blend_dst_in_frag,	// TRANSFER_DST_IN
		blend_dst_out_frag,	// TRANSFER_DST_OUT
		blend_dst_over_frag,	// TRANSFER_DST_OVER
		blend_src_frag,		// TRANSFER_SRC
		blend_src_atop_frag,	// TRANSFER_SRC_ATOP
		blend_src_in_frag,	// TRANSFER_SRC_IN
		blend_src_out_frag,	// TRANSFER_SRC_OUT
		blend_src_over_frag,	// TRANSFER_SRC_OVER
		blend_or_frag,		// TRANSFER_OR
		blend_xor_frag		// TRANSFER_XOR
	};

	glDisable(GL_BLEND);
	VFrame *dst = get_output(output_layer);
        VFrame *src = temp;

	switch( config.mode ) {
	case TRANSFER_REPLACE:
	case TRANSFER_SRC:
// Direct copy layer
        	src->to_texture();
	       	dst->enable_opengl();
		dst->init_screen();
		src->draw_texture();
		break;
	case TRANSFER_NORMAL:
// Move destination to screen
		if( dst->get_opengl_state() != VFrame::SCREEN ) {
			dst->to_texture();
        		dst->enable_opengl();
		        dst->init_screen();
			dst->draw_texture();
		}
        	src->to_texture();
	       	dst->enable_opengl();
		dst->init_screen();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		src->draw_texture();
		break;
	default:
        	src->to_texture();
		dst->to_texture();
	       	dst->enable_opengl();
		dst->init_screen();
        	src->bind_texture(0);
	        dst->bind_texture(1);
		const char *shader_stack[] = { 0, 0, 0 };
		int current_shader = 0;

		shader_stack[current_shader++] = get_pixels_frag;
		shader_stack[current_shader++] = overlay_shaders[config.mode];
		shader_stack[current_shader++] = put_pixels_frag;

		unsigned int shader_id = 0;
		shader_id = VFrame::make_shader(0,
			shader_stack[0],
			shader_stack[1],
			shader_stack[2],
			0);

		glUseProgram(shader_id);
		glUniform1i(glGetUniformLocation(shader_id, "src_tex"), 0);
		glUniform1i(glGetUniformLocation(shader_id, "dst_tex"), 1);
		glUniform2f(glGetUniformLocation(shader_id, "dst_tex_dimensions"),
				(float)dst->get_texture_w(), (float)dst->get_texture_h());
		float chroma_offset = BC_CModels::is_yuv(dst->get_color_model()) ? 0.5 : 0.0;
		glUniform3f(glGetUniformLocation(shader_id, "chroma_offset"),
				0.0, chroma_offset, chroma_offset);

		src->draw_texture();
		glUseProgram(0);

		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		break;
	}

	glActiveTexture(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

// get the data before something else uses the screen
	dst->screen_to_ram();
#endif
	return 0;
}


const char* Overlay::plugin_title() { return _("Overlay"); }
int Overlay::is_realtime() { return 1; }
int Overlay::is_multichannel() { return 1; }
int Overlay::is_synthesis() { return 1; }



NEW_WINDOW_MACRO(Overlay, OverlayWindow)



int Overlay::load_configuration()
{
	KeyFrame *prev_keyframe;
	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return 0;
}


void Overlay::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("OVERLAY");
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("DIRECTION", config.direction);
	output.tag.set_property("OUTPUT_LAYER", config.output_layer);
	output.append_tag();
	output.tag.set_title("/OVERLAY");
	output.append_tag();
	output.terminate_string();
}

void Overlay::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->get_data(), strlen(keyframe->get_data()));

	while(!input.read_tag())
	{
		if(input.tag.title_is("OVERLAY"))
		{
			config.mode = input.tag.get_property("MODE", config.mode);
			config.direction = input.tag.get_property("DIRECTION", config.direction);
			config.output_layer = input.tag.get_property("OUTPUT_LAYER", config.output_layer);
		}
	}
}

void Overlay::update_gui()
{
	if(thread)
	{
		thread->window->lock_window("Overlay::update_gui");
		((OverlayWindow*)thread->window)->mode->set_text(OverlayConfig::mode_to_text(config.mode));
		((OverlayWindow*)thread->window)->direction->set_text(OverlayConfig::direction_to_text(config.direction));
		((OverlayWindow*)thread->window)->output->set_text(OverlayConfig::output_to_text(config.output_layer));
		thread->window->unlock_window();
	}
}





