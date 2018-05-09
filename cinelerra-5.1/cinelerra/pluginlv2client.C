#ifdef HAVE_LV2

/*
 * CINELERRA
 * Copyright (C) 2018 GG
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
#include "cstrdup.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "mwindow.h"
#include "pluginlv2client.h"
#include "pluginserver.h"
#include "samples.h"

#include <ctype.h>
#include <string.h>

#define LV2_SEQ_SIZE  9624

PluginLV2UriTable::PluginLV2UriTable()
{
	set_array_delete();
}

PluginLV2UriTable::~PluginLV2UriTable()
{
	remove_all_objects();
}

LV2_URID PluginLV2UriTable::map(const char *uri)
{
	for( int i=0; i<size(); ++i )
		if( !strcmp(uri, get(i)) ) return i+1;
	append(cstrdup(uri));
	return size();
}

const char *PluginLV2UriTable::unmap(LV2_URID urid)
{
	int idx = urid - 1;
	return idx>=0 && idx<size() ? get(idx) : 0;
}

PluginLV2Client_OptName:: PluginLV2Client_OptName(PluginLV2Client_Opt *opt)
{
	this->opt = opt;
	set_text(opt->get_name());
}

PluginLV2Client_OptValue::PluginLV2Client_OptValue(PluginLV2Client_Opt *opt)
{
	this->opt = opt;
	update();
}

int PluginLV2Client_OptValue::update()
{
	char val[BCSTRLEN];
	sprintf(val, "%f", opt->get_value());
	if( !strcmp(val, get_text()) ) return 0;
	set_text(val);
	return 1;
}


PluginLV2Client_Opt::PluginLV2Client_Opt(PluginLV2ClientConfig *conf, const char *sym, int idx)
{
	this->conf = conf;
	this->idx = idx;
	this->sym = cstrdup(sym);
	item_name = new PluginLV2Client_OptName(this);
	item_value = new PluginLV2Client_OptValue(this);
}

PluginLV2Client_Opt::~PluginLV2Client_Opt()
{
	delete [] sym;
	delete item_name;
	delete item_value;
}

float PluginLV2Client_Opt::get_value()
{
	return conf->ctls[idx];
}

void PluginLV2Client_Opt::set_value(float v)
{
	conf->ctls[idx] = v;
}

int PluginLV2Client_Opt::update(float v)
{
	set_value(v);
	return item_value->update();
}

const char *PluginLV2Client_Opt::get_name()
{
	return conf->names[idx];
}

PluginLV2ClientConfig::PluginLV2ClientConfig()
{
	names = 0;
	mins = 0;
	maxs = 0;
	ctls = 0;
	nb_ports = 0;
}

PluginLV2ClientConfig::~PluginLV2ClientConfig()
{
	reset();
	remove_all_objects();
}

void PluginLV2ClientConfig::reset()
{
	for( int i=0; i<nb_ports; ++i ) delete [] names[i];
	delete [] names; names = 0;
	delete [] mins;  mins = 0;
	delete [] maxs;  maxs = 0;
	delete [] ctls;  ctls = 0;
	nb_ports = 0;
}


int PluginLV2ClientConfig::equivalent(PluginLV2ClientConfig &that)
{
	PluginLV2ClientConfig &conf = *this;
	for( int i=0; i<that.size(); ++i ) {
		PluginLV2Client_Opt *topt = conf[i], *vopt = that[i];
		if( !EQUIV(topt->get_value(), vopt->get_value()) ) return 0;
	}
	return 1;
}

void PluginLV2ClientConfig::copy_from(PluginLV2ClientConfig &that)
{
	if( nb_ports != that.nb_ports ) {
		reset();
		nb_ports = that.nb_ports;
		names = new const char *[nb_ports];
		for( int i=0; i<nb_ports; ++i ) names[i] = 0;
		mins  = new float[nb_ports];
		maxs  = new float[nb_ports];
		ctls  = new float[nb_ports];
	}
	for( int i=0; i<nb_ports; ++i ) {
		delete [] names[i];
		names[i] = cstrdup(that.names[i]);
		mins[i] = that.mins[i];
		maxs[i] = that.maxs[i];
		ctls[i] = that.ctls[i];
	}
	remove_all_objects();
	for( int i=0; i<that.size(); ++i ) {
		append(new PluginLV2Client_Opt(this, that[i]->sym, that[i]->idx));
	}
}

void PluginLV2ClientConfig::interpolate(PluginLV2ClientConfig &prev, PluginLV2ClientConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}

void PluginLV2ClientConfig::init_lv2(const LilvPlugin *lilv)
{
	reset();
	nb_ports = lilv_plugin_get_num_ports(lilv);
	names = new const char *[nb_ports];
	mins  = new float[nb_ports];
	maxs  = new float[nb_ports];
	ctls  = new float[nb_ports];
	lilv_plugin_get_port_ranges_float(lilv, mins, maxs, ctls);
	for( int i=0; i<nb_ports; ++i ) {
		const LilvPort *lp = lilv_plugin_get_port_by_index(lilv, i);
		LilvNode *pnm = lilv_port_get_name(lilv, lp);
		names[i] = cstrdup(lilv_node_as_string(pnm));
		lilv_node_free(pnm);
	}
}

int PluginLV2ClientConfig::update()
{
	int ret = 0;
	PluginLV2ClientConfig &conf = *this;
	for( int i=0; i<size(); ++i ) {
		if( conf[i]->item_value->update() ) ++ret;
	}
	return ret;
}

PluginLV2ClientReset::
PluginLV2ClientReset(PluginLV2ClientWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->gui = gui;
}

PluginLV2ClientReset::
~PluginLV2ClientReset()
{
}

int PluginLV2ClientReset::handle_event()
{
	PluginLV2Client *plugin = gui->plugin;
	plugin->init_lv2();
	gui->selected = 0;
	gui->update_selected();
	gui->panel->update();
	plugin->send_configure_change();
	return 1;
}

PluginLV2ClientText::
PluginLV2ClientText(PluginLV2ClientWindow *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, (char *)"")
{
	this->gui = gui;
}

PluginLV2ClientText::
~PluginLV2ClientText()
{
}

int PluginLV2ClientText::handle_event()
{
	return 0;
}


PluginLV2ClientApply::
PluginLV2ClientApply(PluginLV2ClientWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->gui = gui;
}

PluginLV2ClientApply::
~PluginLV2ClientApply()
{
}

int PluginLV2ClientApply::handle_event()
{
	const char *text = gui->text->get_text();
	if( text && gui->selected ) {
		gui->selected->update(atof(text));
		gui->update_selected();
		gui->plugin->send_configure_change();
	}
	return 1;
}


PluginLV2Client_OptPanel::
PluginLV2Client_OptPanel(PluginLV2ClientWindow *gui, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT), opts(items[0]), vals(items[1])
{
	this->gui = gui;
	update();  // init col/wid/columns
}

PluginLV2Client_OptPanel::
~PluginLV2Client_OptPanel()
{
}

int PluginLV2Client_OptPanel::selection_changed()
{
	PluginLV2Client_Opt *opt = 0;
	BC_ListBoxItem *item = get_selection(0, 0);
	if( item ) {
		PluginLV2Client_OptName *opt_name = (PluginLV2Client_OptName *)item;
		opt = opt_name->opt;
	}
	gui->update(opt);
	return 1;
}

void PluginLV2Client_OptPanel::update()
{
	opts.remove_all();
	vals.remove_all();
	PluginLV2ClientConfig &conf = gui->plugin->config;
	for( int i=0; i<conf.size(); ++i ) {
		PluginLV2Client_Opt *opt = conf[i];
		opts.append(opt->item_name);
		vals.append(opt->item_value);
	}
	const char *cols[] = { "option", "value", };
	const int col1_w = 150;
	int wids[] = { col1_w, get_w()-col1_w };
	BC_ListBox::update(&items[0], &cols[0], &wids[0], sizeof(items)/sizeof(items[0]));
}

PluginLV2ClientWindow::PluginLV2ClientWindow(PluginLV2Client *plugin)
 : PluginClientWindow(plugin, 500, 300, 500, 300, 1)
{
	this->plugin = plugin;
	selected = 0;
}

PluginLV2ClientWindow::~PluginLV2ClientWindow()
{
}


void PluginLV2ClientWindow::create_objects()
{
	BC_Title *title;
	int x = 10, y = 10;
	add_subwindow(title = new BC_Title(x, y, plugin->title));
	y += title->get_h() + 10;
	add_subwindow(varbl = new BC_Title(x, y, ""));
	add_subwindow(range = new BC_Title(x+160, y, ""));
	int x1 = get_w() - BC_GenericButton::calculate_w(this, _("Reset")) - 8;
	add_subwindow(reset = new PluginLV2ClientReset(this, x1, y));
	y += title->get_h() + 10;
	x1 = get_w() - BC_GenericButton::calculate_w(this, _("Apply")) - 8;
	add_subwindow(apply = new PluginLV2ClientApply(this, x1, y));
	add_subwindow(text = new PluginLV2ClientText(this, x, y, x1-x - 8));
	y += title->get_h() + 10;
	add_subwindow(pot = new PluginLV2ClientPot(this, x, y));
	x1 = x + pot->get_w() + 10;
	add_subwindow(slider = new PluginLV2ClientSlider(this, x1, y+10));
	y += pot->get_h() + 10;

	plugin->init_lv2();

	int panel_x = x, panel_y = y;
	int panel_w = get_w()-10 - panel_x;
	int panel_h = get_h()-10 - panel_y;
	panel = new PluginLV2Client_OptPanel(this, panel_x, panel_y, panel_w, panel_h);
	add_subwindow(panel);
	panel->update();
	show_window(1);
}

int PluginLV2ClientWindow::resize_event(int w, int h)
{
	int x1 = w - reset->get_w() - 8;
	reset->reposition_window(x1, reset->get_y());
	x1 = w - apply->get_w() - 8;
	apply->reposition_window(x1, apply->get_y());
	text->reposition_window(text->get_x(), text->get_y(), x1-text->get_x() - 8);
	x1 = pot->get_x() + pot->get_w() + 10;
	int w1 = w - slider->get_x() - 20;
	slider->set_pointer_motion_range(w1);
	slider->reposition_window(x1, slider->get_y(), w1, slider->get_h());
	int panel_x = panel->get_x(), panel_y = panel->get_y();
	panel->reposition_window(panel_x, panel_y, w-10-panel_x, h-10-panel_y);
	return 1;
}

PluginLV2ClientPot::PluginLV2ClientPot(PluginLV2ClientWindow *gui, int x, int y)
 : BC_FPot(x, y, 0.f, 0.f, 0.f)
{
	this->gui = gui;
}

int PluginLV2ClientPot::handle_event()
{
	if( gui->selected ) {
		gui->selected->update(get_value());
		gui->update_selected();
		gui->plugin->send_configure_change();
	}
	return 1;
}

PluginLV2ClientSlider::PluginLV2ClientSlider(PluginLV2ClientWindow *gui, int x, int y)
 : BC_FSlider(x, y, 0, gui->get_w()-x-20, gui->get_w()-x-20, 0.f, 0.f, 0.f)
{
	this->gui = gui;
}

int PluginLV2ClientSlider::handle_event()
{
	if( gui->selected ) {
		gui->selected->update(get_value());
		gui->update_selected();
		gui->plugin->send_configure_change();
	}
	return 1;
}

void PluginLV2ClientWindow::update_selected()
{
	update(selected);
}

int PluginLV2ClientWindow::scalar(float f, char *rp)
{
	const char *cp = 0;
	     if( f == FLT_MAX ) cp = "FLT_MAX";
	else if( f == FLT_MIN ) cp = "FLT_MIN";
	else if( f == -FLT_MAX ) cp = "-FLT_MAX";
	else if( f == -FLT_MIN ) cp = "-FLT_MIN";
	else if( f == 0 ) cp = signbit(f) ? "-0" : "0";
	else if( isnan(f) ) cp = signbit(f) ? "-NAN" : "NAN";
	else if( isinf(f) ) cp = signbit(f) ? "-INF" : "INF";
	else return sprintf(rp, "%g", f);
	return sprintf(rp, "%s", cp);
}

void PluginLV2ClientWindow::update(PluginLV2Client_Opt *opt)
{
	if( selected != opt ) {
		if( selected ) selected->item_name->set_selected(0);
		selected = opt;
		if( selected ) selected->item_name->set_selected(1);
	}
	char var[BCSTRLEN];  var[0] = 0;
	char val[BCSTRLEN];  val[0] = 0;
	char rng[BCTEXTLEN]; rng[0] = 0;
	if( opt ) {
		sprintf(var,"%s:", opt->conf->names[opt->idx]);
		char *cp = rng;
		cp += sprintf(cp,"( ");
		float min = opt->conf->mins[opt->idx];
		cp += scalar(min, cp);
		cp += sprintf(cp, " .. ");
		float max = opt->conf->maxs[opt->idx];
		cp += scalar(max, cp);
		cp += sprintf(cp, " )");
		float v = opt->get_value();
		sprintf(val, "%f", v);
		slider->update(slider->get_w(), v, min, max);
		pot->update(v, min, max);
	}
	else {
		slider->update(slider->get_w(), 0.f, 0.f, 0.f);
		pot->update(0.f, 0.f, 0.f);
	}
	varbl->update(var);
	range->update(rng);
	text->update(val);
	panel->update();
}


PluginLV2Client::PluginLV2Client(PluginServer *server)
 : PluginAClient(server)
{
	in_buffers = 0;
	out_buffers = 0;
	nb_in_bfrs = 0;
	nb_out_bfrs = 0;
	bfrsz = 0;
	nb_inputs = 0;
	nb_outputs = 0;
	max_bufsz = 0;

	world = 0;
	instance = 0;
	lv2_InputPort = 0;
	lv2_OutputPort = 0;
	lv2_AudioPort = 0;
	lv2_CVPort = 0;
	lv2_ControlPort = 0;
	lv2_Optional = 0;
	atom_AtomPort = 0;
	atom_Sequence = 0;
	urid_map = 0;
	powerOf2BlockLength = 0;
	fixedBlockLength = 0;
	boundedBlockLength = 0;
	seq_out = 0;
}

PluginLV2Client::~PluginLV2Client()
{
	reset_lv2();
	lilv_world_free(world);
}

void PluginLV2Client::reset_lv2()
{
	if( instance ) lilv_instance_deactivate(instance);
	lilv_instance_free(instance);         instance = 0;
	lilv_node_free(powerOf2BlockLength);  powerOf2BlockLength = 0;
	lilv_node_free(fixedBlockLength);     fixedBlockLength = 0;
	lilv_node_free(boundedBlockLength);   boundedBlockLength = 0;
	lilv_node_free(urid_map);             urid_map = 0;
	lilv_node_free(atom_Sequence);        atom_Sequence = 0;
	lilv_node_free(atom_AtomPort);        atom_AtomPort = 0;
	lilv_node_free(lv2_Optional);         lv2_Optional = 0;
	lilv_node_free(lv2_ControlPort);      lv2_ControlPort = 0;
	lilv_node_free(lv2_AudioPort);        lv2_AudioPort = 0;
	lilv_node_free(lv2_CVPort);           lv2_CVPort = 0;
	lilv_node_free(lv2_OutputPort);       lv2_OutputPort = 0;
	lilv_node_free(lv2_InputPort);        lv2_InputPort = 0;
	delete [] (char *)seq_out;            seq_out = 0;
	uri_table.remove_all_objects();
	delete_buffers();
	nb_inputs = 0;
	nb_outputs = 0;
	max_bufsz = 0;
	config.reset();
	config.remove_all_objects();
}

int PluginLV2Client::load_lv2(const char *path)
{
	if( !world ) {
		world = lilv_world_new();
		if( !world ) {
			printf("lv2: lilv_world_new failed");
			return 1;
		}
		lilv_world_load_all(world);
	}

	LilvNode *uri = lilv_new_uri(world, path);
	if( !uri ) {
		printf("lv2: lilv_new_uri(%s) failed", path);
		return 1;
	}

	const LilvPlugins *all_plugins = lilv_world_get_all_plugins(world);
	lilv = lilv_plugins_get_by_uri(all_plugins, uri);
	lilv_node_free(uri);
	if( !lilv ) {
		printf("lv2: lilv_plugins_get_by_uriPlugin(%s) failed", path);
		return 1;
	}

	LilvNode *name = lilv_plugin_get_name(lilv);
	const char *nm = lilv_node_as_string(name);
	snprintf(title,sizeof(title),"L2_%s",nm);
	lilv_node_free(name);
	return 0;
}

int PluginLV2Client::init_lv2()
{
	reset_lv2();

	lv2_InputPort       = lilv_new_uri(world, LV2_CORE__InputPort);
	lv2_OutputPort      = lilv_new_uri(world, LV2_CORE__OutputPort);
	lv2_AudioPort       = lilv_new_uri(world, LV2_CORE__AudioPort);
	lv2_ControlPort     = lilv_new_uri(world, LV2_CORE__ControlPort);
	lv2_CVPort          = lilv_new_uri(world, LV2_CORE__CVPort);
	lv2_Optional        = lilv_new_uri(world, LV2_CORE__connectionOptional);
	atom_AtomPort       = lilv_new_uri(world, LV2_ATOM__AtomPort);
	atom_Sequence       = lilv_new_uri(world, LV2_ATOM__Sequence);
	urid_map            = lilv_new_uri(world, LV2_URID__map);
	powerOf2BlockLength = lilv_new_uri(world, LV2_BUF_SIZE__powerOf2BlockLength);
	fixedBlockLength    = lilv_new_uri(world, LV2_BUF_SIZE__fixedBlockLength);
	boundedBlockLength  = lilv_new_uri(world, LV2_BUF_SIZE__boundedBlockLength);
	seq_out = (LV2_Atom_Sequence *) new char[sizeof(LV2_Atom_Sequence) + LV2_SEQ_SIZE];

	config.init_lv2(lilv);
	nb_inputs = nb_outputs = 0;

	for( int i=0; i<config.nb_ports; ++i ) {
		const LilvPort *lp = lilv_plugin_get_port_by_index(lilv, i);
		int is_input = lilv_port_is_a(lilv, lp, lv2_InputPort);
		if( !is_input && !lilv_port_is_a(lilv, lp, lv2_OutputPort) &&
		    !lilv_port_has_property(lilv, lp, lv2_Optional) ) {
			printf("lv2: not input, not output, and not optional: %s\n", config.names[i]);
			continue;
		}
		if( is_input && lilv_port_is_a(lilv, lp, lv2_ControlPort) ) {
			const char *sym = lilv_node_as_string(lilv_port_get_symbol(lilv, lp));
			config.append(new PluginLV2Client_Opt(&config, sym, i));
			continue;
		}
		if( lilv_port_is_a(lilv, lp, lv2_AudioPort) ||
		    lilv_port_is_a(lilv, lp, lv2_CVPort ) ) {
			if( is_input ) ++nb_inputs; else ++nb_outputs;
			continue;
		}
	}

	map.handle = (void*)&uri_table;
	map.map = uri_table_map;
	map_feature.URI = LV2_URID_MAP_URI;
	map_feature.data = &map;
	unmap.handle = (void*)&uri_table;
	unmap.unmap  = uri_table_unmap;
	unmap_feature.URI = LV2_URID_UNMAP_URI;
	unmap_feature.data = &unmap;
	features[0] = &map_feature;
	features[1] = &unmap_feature;
	static const LV2_Feature buf_size_features[3] = {
		{ LV2_BUF_SIZE__powerOf2BlockLength, NULL },
		{ LV2_BUF_SIZE__fixedBlockLength,    NULL },
		{ LV2_BUF_SIZE__boundedBlockLength,  NULL },
	};
	features[2] = &buf_size_features[0];
	features[3] = &buf_size_features[1];
	features[4] = &buf_size_features[2];
	features[5] = 0;

	instance = lilv_plugin_instantiate(lilv, sample_rate, features);
	if( !instance ) {
		printf("lv2: lilv_plugin_instantiate failed: %s\n", server->title);
		return 1;
	}
	lilv_instance_activate(instance);
// not sure what to do with these
	max_bufsz = nb_inputs &&
		(lilv_plugin_has_feature(lilv, powerOf2BlockLength) ||
		 lilv_plugin_has_feature(lilv, fixedBlockLength) ||
		 lilv_plugin_has_feature(lilv, boundedBlockLength)) ? 4096 : 0;
	return 0;
}

LV2_URID PluginLV2Client::uri_table_map(LV2_URID_Map_Handle handle, const char *uri)
{
	return ((PluginLV2UriTable *)handle)->map(uri);
}

const char *PluginLV2Client::uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
{
	return ((PluginLV2UriTable *)handle)->unmap(urid);
}

NEW_WINDOW_MACRO(PluginLV2Client, PluginLV2ClientWindow)

int PluginLV2Client::load_configuration()
{
	int64_t src_position =  get_source_position();
	KeyFrame *prev_keyframe = get_prev_keyframe(src_position);
	PluginLV2ClientConfig curr_config;
	curr_config.copy_from(config);
	read_data(prev_keyframe);
	int ret = !config.equivalent(curr_config) ? 1 : 0;
	return ret;
}

void PluginLV2Client::update_gui()
{
	PluginClientThread *thread = get_thread();
	if( !thread ) return;
	PluginLV2ClientWindow *window = (PluginLV2ClientWindow*)thread->get_window();
	window->lock_window("PluginFClient::update_gui");
	load_configuration();
	if( config.update() > 0 )
		window->update_selected();
	window->unlock_window();
}

int PluginLV2Client::is_realtime() { return 1; }
int PluginLV2Client::is_multichannel() { return nb_inputs > 1 || nb_outputs > 1 ? 1 : 0; }
const char* PluginLV2Client::plugin_title() { return title; }
int PluginLV2Client::uses_gui() { return 1; }
int PluginLV2Client::is_synthesis() { return 1; }

char* PluginLV2Client::to_string(char *string, const char *input)
{
	char *bp = string;
	for( const char *cp=input; *cp; ++cp )
		*bp++ = isalnum(*cp) ? *cp : '_';
	*bp = 0;
	return string;
}

void PluginLV2Client::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->get_data(), MESSAGESIZE);
	char name[BCTEXTLEN];  to_string(name, plugin_title());
	output.tag.set_title(name);
	for( int i=0; i<config.size(); ++i ) {
		PluginLV2Client_Opt *opt = config[i];
		output.tag.set_property(opt->sym, opt->get_value());
	}
	output.append_tag();
	output.terminate_string();
}

void PluginLV2Client::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->get_data(), strlen(keyframe->get_data()));
	char name[BCTEXTLEN];  to_string(name, plugin_title());

	while( !input.read_tag() ) {
		if( !input.tag.title_is(name) ) continue;
		for( int i=0; i<config.size(); ++i ) {
			PluginLV2Client_Opt *opt = config[i];
			float value = input.tag.get_property(opt->sym, 0.);
			opt->set_value(value);
		}
	}
}

void PluginLV2Client::connect_ports()
{
	int ich = 0, och = 0;
	for( int i=0; i<config.nb_ports; ++i ) {
		const LilvPort *lp = lilv_plugin_get_port_by_index(lilv, i);
		if( lilv_port_is_a(lilv, lp, lv2_AudioPort) ||
		    lilv_port_is_a(lilv, lp, lv2_CVPort) ) {
			if( lilv_port_is_a(lilv, lp, lv2_InputPort) )
				lilv_instance_connect_port(instance, i, in_buffers[ich++]);
			else if( lilv_port_is_a(lilv, lp, lv2_OutputPort))
				lilv_instance_connect_port(instance, i, out_buffers[och++]);
			continue;
		}
		if( lilv_port_is_a(lilv, lp, atom_AtomPort) ) {
			if( lilv_port_is_a(lilv, lp, lv2_InputPort) )
				lilv_instance_connect_port(instance, i, &seq_in);
			else
				lilv_instance_connect_port(instance, i, seq_out);
			continue;
		}
		if( lilv_port_is_a(lilv, lp, lv2_ControlPort) ) {
			lilv_instance_connect_port(instance, i, &config.ctls[i]);
			continue;
		}
	}

	seq_in[0].atom.size = sizeof(LV2_Atom_Sequence_Body);
	seq_in[0].atom.type = uri_table.map(LV2_ATOM__Sequence);
	seq_in[1].atom.size = 0;
	seq_in[1].atom.type = 0;
	seq_out->atom.size  = LV2_SEQ_SIZE;
	seq_out->atom.type  = uri_table.map(LV2_ATOM__Chunk);
}

void PluginLV2Client::init_plugin(int size)
{
	if( !instance )
		init_lv2();

	load_configuration();

	if( bfrsz < size )
		delete_buffers();

	bfrsz = MAX(size, bfrsz);
	if( !in_buffers ) {
		in_buffers = new float*[nb_in_bfrs=nb_inputs];
		for(int i=0; i<nb_in_bfrs; ++i )
			in_buffers[i] = new float[bfrsz];
	}
	if( !out_buffers ) {
		out_buffers = new float*[nb_out_bfrs=nb_outputs];
		for( int i=0; i<nb_out_bfrs; ++i )
			out_buffers[i] = new float[bfrsz];
	}
}

void PluginLV2Client::delete_buffers()
{
	if( in_buffers ) {
		for( int i=0; i<nb_in_bfrs; ++i ) delete [] in_buffers[i];
		delete [] in_buffers;  in_buffers = 0;  nb_in_bfrs = 0;
	}
	if( out_buffers ) {
		for( int i=0; i<nb_out_bfrs; ++i ) delete [] out_buffers[i];
		delete [] out_buffers;  out_buffers = 0;  nb_out_bfrs = 0;
	}
	bfrsz = 0;
}

int PluginLV2Client::process_realtime(int64_t size,
	Samples *input_ptr, Samples *output_ptr)
{
	init_plugin(size);

	for( int i=0; i<nb_in_bfrs; ++i ) {
		double *inp = input_ptr->get_data();
		float *ip = in_buffers[i];
		for( int j=size; --j>=0; *ip++=*inp++ );
	}
	for( int i=0; i<nb_out_bfrs; ++i )
		bzero(out_buffers[i], size*sizeof(float));

	connect_ports();
	lilv_instance_run(instance, size);

	double *outp = output_ptr->get_data();
	float *op = out_buffers[0];
	for( int i=size; --i>=0; *outp++=*op++ );
	return size;
}

int PluginLV2Client::process_realtime(int64_t size,
	Samples **input_ptr, Samples **output_ptr)
{
	init_plugin(size);

	for( int i=0; i<nb_in_bfrs; ++i ) {
		int k = i < PluginClient::total_in_buffers ? i : 0;
		double *inp = input_ptr[k]->get_data();
		float *ip = in_buffers[i];
		for( int j=size; --j>=0; *ip++=*inp++ );
	}
	for( int i=0; i<nb_out_bfrs; ++i )
		bzero(out_buffers[i], size*sizeof(float));

	connect_ports();
	lilv_instance_run(instance, size);

	int nbfrs = PluginClient::total_out_buffers;
	if( nb_out_bfrs < nbfrs ) nbfrs = nb_out_bfrs;
	for( int i=0; i<nbfrs; ++i ) {
		double *outp = output_ptr[i]->get_data();
		float *op = out_buffers[i];
		for( int j=size; --j>=0; *outp++=*op++ );
	}
	return size;
}

PluginServer* MWindow::new_lv2_server(MWindow *mwindow, const char *name)
{
	return new PluginServer(mwindow, name, PLUGIN_TYPE_LV2);
}

PluginClient *PluginServer::new_lv2_plugin()
{
	PluginLV2Client *client = new PluginLV2Client(this);
	if( client->load_lv2(path) ) { delete client;  client = 0; }
	else client->init_lv2();
	return client;
}

int MWindow::init_lv2_index(MWindow *mwindow, Preferences *preferences, FILE *fp)
{
	printf("init lv2 index:\n");
	LilvWorld *world = lilv_world_new();
	lilv_world_load_all(world);
	const LilvPlugins *all_plugins = lilv_world_get_all_plugins(world);

	LILV_FOREACH(plugins, i, all_plugins) {
		const LilvPlugin *lilv = lilv_plugins_get(all_plugins, i);
		const char *uri = lilv_node_as_uri(lilv_plugin_get_uri(lilv));
		PluginServer server(mwindow, uri, PLUGIN_TYPE_LV2);
		int result = server.open_plugin(1, preferences, 0, 0);
		if( !result ) {
			server.write_table(fp, uri, PLUGIN_LV2_ID, 0);
			server.close_plugin();
		}
	}

	lilv_world_free(world);
	return 0;
}

#else
#include "mwindow.h"
#include "pluginserver.h"

PluginServer* MWindow::new_lv2_server(MWindow *mwindow, const char *name) { return 0; }
PluginClient *PluginServer::new_lv2_plugin() { return 0; }
int MWindow::init_lv2_index(MWindow *mwindow, Preferences *preferences, FILE *fp) { return 0; }

#endif /* HAVE_LV2 */
