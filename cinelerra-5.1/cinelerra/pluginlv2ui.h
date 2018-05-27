
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

#ifdef HAVE_LV2UI

#include "forkbase.h"
#include "pluginlv2.h"
#include "pluginlv2client.h"
#include "pluginlv2gui.inc"
#include "pluginlv2ui.inc"

typedef struct _GtkWidget GtkWidget;

#define UPDATE_HOST 1
#define UPDATE_PORTS 2

class PluginLV2UI : public PluginLV2
{
public:
	PluginLV2UI();
	~PluginLV2UI();

	const LilvUI *lilv_ui;
	const LilvNode *lilv_type;

	LV2_Extension_Data_Feature *ext_data;
	PluginLV2UriTable  uri_table;
	LV2_URI_Map_Feature *uri_map;
	LV2_URID_Map       map;
	LV2_URID_Unmap     unmap;

	char title[BCSTRLEN];
	PluginLV2ClientConfig config;
	int updates, hidden;
	int done, running;
	const char *gtk_type;
	GtkWidget *top_level;
	PluginLV2ChildUI *child;

	void reset_gui();
	int init_ui(const char *path, int sample_rate);
	void start();
	void stop();
	int send_host(int64_t token, const void *data, int bytes);
	int update_lv2_input(float *vals, int force);
	void update_lv2_output();

	void run_lilv(int samples);
	void update_value(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr);
	void update_control(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr);
	static void write_from_ui(void *the, uint32_t idx,
		uint32_t bfrsz, uint32_t typ, const void *bfr);
	static uint32_t port_index(void* obj, const char* sym);

//	static void touch(void *obj,uint32_t pidx,bool grabbed);
	static uint32_t uri_to_id(LV2_URID_Map_Handle handle, const char *map, const char *uri);
	static LV2_URID uri_table_map(LV2_URID_Map_Handle handle, const char *uri);
	static const char *uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid);
	void lv2ui_instantiate(void *parent);
	bool lv2ui_resizable();
	void start_gui();
	int run_ui(PluginLV2ChildUI *child=0);
	void update_host();
	int run(int ac, char **av);
};

class PluginLV2ChildUI : public ForkChild, public PluginLV2UI
{
public:
	PluginLV2ChildUI();
	~PluginLV2ChildUI();
	void run();
	void run_buffer(int shmid);

	int handle_child();
	int run(int ac, char **av);
};
#endif
#endif
