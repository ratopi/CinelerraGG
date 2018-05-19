
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

#ifndef PLUGINLV2CLIENT_H
#define PLUGINLV2CLIENT_H

#include "pluginaclient.h"
#include "pluginlv2config.h"
#include "pluginlv2client.inc"
#include "pluginlv2gui.h"
#include "samples.inc"

#define LV2_SEQ_SIZE  9624

class PluginLV2Client_OptPanel : public BC_ListBox
{
public:
	PluginLV2Client_OptPanel(PluginLV2ClientWindow *gui, int x, int y, int w, int h);
	~PluginLV2Client_OptPanel();

	PluginLV2ClientWindow *gui;
	ArrayList<BC_ListBoxItem*> items[2];
	ArrayList<BC_ListBoxItem*> &opts;
	ArrayList<BC_ListBoxItem*> &vals;

	int selection_changed();
	void update();
};

class PluginLV2ClientText : public BC_TextBox {
public:
	PluginLV2ClientWindow *gui;

	PluginLV2ClientText(PluginLV2ClientWindow *gui, int x, int y, int w);
	~PluginLV2ClientText();
	int handle_event();
};

class PluginLV2ClientReset : public BC_GenericButton
{
public:
	PluginLV2ClientWindow *gui;

	PluginLV2ClientReset(PluginLV2ClientWindow *gui, int x, int y);
	~PluginLV2ClientReset();
	int handle_event();
};

class PluginLV2ClientUI : public BC_GenericButton {
public:
	PluginLV2ClientWindow *gui;

	PluginLV2ClientUI(PluginLV2ClientWindow *gui, int x, int y);
	~PluginLV2ClientUI();
	int handle_event();
};

class PluginLV2ClientApply : public BC_GenericButton {
public:
	PluginLV2ClientWindow *gui;

	PluginLV2ClientApply(PluginLV2ClientWindow *gui, int x, int y);
	~PluginLV2ClientApply();
	int handle_event();
};

class PluginLV2ClientPot : public BC_FPot
{
public:
	PluginLV2ClientPot(PluginLV2ClientWindow *gui, int x, int y);
	int handle_event();
	PluginLV2ClientWindow *gui;
};

class PluginLV2ClientSlider : public BC_FSlider
{
public:
	PluginLV2ClientSlider(PluginLV2ClientWindow *gui, int x, int y);
	int handle_event();
	PluginLV2ClientWindow *gui;
};

class PluginLV2ClientWindow : public PluginClientWindow
{
public:
	PluginLV2ClientWindow(PluginLV2Client *plugin);
	~PluginLV2ClientWindow();

	void create_objects();
	int resize_event(int w, int h);
	void update_selected();
	void update_selected(float v);
	int scalar(float f, char *rp);
	void update(PluginLV2Client_Opt *opt);

	PluginLV2Client *plugin;
	PluginLV2ClientUI *ui;
	PluginLV2ClientReset *reset;
	PluginLV2ClientApply *apply;
	PluginLV2ClientPot *pot;
	PluginLV2ClientSlider *slider;
	PluginLV2ClientText *text;
	BC_Title *varbl, *range;
	PluginLV2Client_OptPanel *panel;
	PluginLV2Client_Opt *selected;
};


class PluginLV2Client : public PluginAClient
{
public:
	PluginLV2Client(PluginServer *server);
	~PluginLV2Client();

	int process_realtime(int64_t size,
		Samples *input_ptr,
		Samples *output_ptr);
	int process_realtime(int64_t size,
		Samples **input_ptr,
		Samples **output_ptr);
// Update output pointers as well
	int is_realtime();
	int is_multichannel();
	int is_synthesis();
	int uses_gui();
	char* to_string(char *string, const char *input);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void reset_lv2();
	int load_lv2(const char *path);
	int init_lv2();
	int open_lv2_gui(PluginLV2ClientWindow *gui);
	void update_gui();
	void update_lv2();
	void lv2_update();
	void lv2_update(float *vals);
	void lv2_set(int idx, float val);

	PLUGIN_CLASS_MEMBERS(PluginLV2ClientConfig)
	char title[BCSTRLEN];
	int bfrsz, nb_in_bfrs, nb_out_bfrs;
	float **in_buffers, **out_buffers;

	void delete_buffers();
	void init_plugin(int size);
	void connect_ports();
	static LV2_URID uri_table_map(LV2_URID_Map_Handle handle, const char *uri);
	static const char *uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid);

// lv2
	LilvWorld         *world;
	const LilvPlugin  *lilv;
	int nb_inputs, nb_outputs;
	int max_bufsz;

	PluginLV2UriTable  uri_table;
	LV2_URID_Map       map;
	LV2_Feature        map_feature;
	LV2_URID_Unmap     unmap;
	LV2_Feature        unmap_feature;
	Lv2Features        features;
	LV2_Atom_Sequence  seq_in[2];
	LV2_Atom_Sequence  *seq_out;

	LilvInstance *instance;
	LilvNode *atom_AtomPort;
	LilvNode *atom_Sequence;
	LilvNode *lv2_AudioPort;
	LilvNode *lv2_CVPort;
	LilvNode *lv2_ControlPort;
	LilvNode *lv2_Optional;
	LilvNode *lv2_InputPort;
	LilvNode *lv2_OutputPort;
	LilvNode *powerOf2BlockLength;
	LilvNode *fixedBlockLength;
	LilvNode *boundedBlockLength;

	PluginLV2ParentGUI *parent_gui;
};

#endif
