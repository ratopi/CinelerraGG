
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

#ifndef __PLUGINLV2UI_H__
#define __PLUGINLV2UI_H__

#include "forkbase.h"
#include "pluginlv2gui.inc"
#include "pluginlv2client.h"

class PluginLV2GUI
{
public:
	PluginLV2GUI();
	~PluginLV2GUI();

	void reset_gui(void);
	int init_gui(const char *path);
	void update_value(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr);
	static void write_from_ui(void *the, uint32_t idx, uint32_t bfrsz,uint32_t typ,const void *bfr);
	void update_control(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr);
	static uint32_t port_index(void *obj,const char *sym);
	static void touch(void *obj,uint32_t pidx,bool grabbed);
	static uint32_t uri_to_id(LV2_URID_Map_Handle handle, const char *map, const char *uri);
	static LV2_URID uri_table_map(LV2_URID_Map_Handle handle, const char *uri);
	static const char *uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid);
	void lv2ui_instantiate(void *parent);
	bool lv2ui_resizable();
	void host_update(PluginLV2ChildGUI *child);
	int run(int ac, char **av);

	PluginLV2ClientConfig config;
	PluginLV2UriTable  uri_table;
	LV2_URI_Map_Feature *uri_map;
	LV2_Extension_Data_Feature *ext_data;
	LV2_URID_Map       map;
	LV2_URID_Unmap     unmap;
	Lv2Features        ui_features;
	LilvNode *lv2_InputPort;
	LilvNode *lv2_ControlPort;

	LilvWorld *world;
        const LilvPlugin *lilv;
	LilvInstance *inst;
        LilvUIs* uis;
	const LilvUI *ui;
        const LilvNode *ui_type;

#ifdef HAVE_LV2UI
	SuilInstance *sinst;
	SuilHost *ui_host;
#endif

	char title[BCSTRLEN];
	const char *gtk_type;
	uint32_t last, updates;
	int done, running;

	void start();
	void stop();
	int update_lv2(float *vals, int force);
	int redraw;

	void run_gui(PluginLV2ChildGUI *child=0);
};

enum { NO_COMMAND,
	LV2_OPEN,
	LV2_LOAD,
	LV2_UPDATE,
	LV2_START,
	LV2_SET,
	NB_COMMANDS };

typedef struct { int idx;  float value; } control_t;

class PluginLV2ParentGUI : public ForkParent
{
public:
	PluginLV2ParentGUI(PluginLV2ClientWindow *gui);
	~PluginLV2ParentGUI();
	ForkChild* new_fork();
	void start_parent();

	int handle_parent();
	PluginLV2ClientWindow *gui;
};

class PluginLV2ChildGUI : public ForkChild
{
public:
	PluginLV2ChildGUI();
	~PluginLV2ChildGUI();

	int handle_child();
	void run();
	int run(int ac, char **av);

	PluginLV2GUI *lv2_gui;
// Command tokens
};

#endif
