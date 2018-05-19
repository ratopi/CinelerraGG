#ifdef HAVE_LV2UI

#include "file.h"
#include "language.h"
#include "pluginlv2gui.h"

#include <ctype.h>
#include <string.h>


PluginLV2ChildGUI::PluginLV2ChildGUI()
{
	lv2_gui = 0;
}

PluginLV2ChildGUI::~PluginLV2ChildGUI()
{
	delete lv2_gui;
}

void PluginLV2ChildGUI::run()
{
	ArrayList<char *> av;
	av.set_array_delete();
	char arg[BCTEXTLEN];
	const char *exec_path = File::get_cinlib_path();
	snprintf(arg, sizeof(arg), "%s/%s", exec_path, "lv2ui");
	av.append(cstrdup(arg));
	sprintf(arg, "%d", child_fd);
	av.append(cstrdup(arg));
	sprintf(arg, "%d", parent_fd);
	av.append(cstrdup(arg));
	av.append(0);
	execv(av[0], &av.values[0]);
	fprintf(stderr, "execv failed: %s\n %m\n", av.values[0]);
	av.remove_all_objects();
	_exit(1);
}


#define NS_EXT "http://lv2plug.in/ns/ext/"

PluginLV2GUI::PluginLV2GUI()
{
	world = 0;
	lilv = 0;
	uri_map = 0;
	ext_data = 0;
	inst = 0;
	sinst = 0;
	ui_host = 0;
	uis = 0;
	ui = 0;
	ui_type = 0;
	lv2_InputPort = 0;
	lv2_ControlPort = 0;

	updates = 0;
	last = 0;
	done = 0;
	running = 0;
	redraw = 0;

// only gtk-2
	gtk_type = "http://lv2plug.in/ns/extensions/ui#GtkUI";
}

PluginLV2GUI::~PluginLV2GUI ()
{
	reset_gui();
	if( world ) lilv_world_free(world);
}

void PluginLV2GUI::reset_gui()
{
	if( inst ) lilv_instance_deactivate(inst);
	if( uri_map )   { delete uri_map;             uri_map = 0; }
	if( ext_data )  { delete ext_data;            ext_data = 0; }
	if( inst )      { lilv_instance_free(inst);   inst = 0; }
	if( sinst )     { suil_instance_free(sinst);  sinst = 0; }
	if( ui_host )   { suil_host_free(ui_host);    ui_host = 0; }
	if( uis )       { lilv_uis_free(uis);         uis = 0; }
	if( lv2_InputPort )  { lilv_node_free(lv2_InputPort);   lv2_InputPort = 0; }
	if( lv2_ControlPort ) { lilv_node_free(lv2_ControlPort);  lv2_ControlPort = 0; }
	ui_features.remove_all_objects();
	uri_table.remove_all_objects();
	config.reset();
	config.remove_all_objects();
}

int PluginLV2GUI::init_gui(const char *path)
{
	reset_gui();
	if( !world )
		world = lilv_world_new();
	if( !world ) {
		printf("lv2_gui: lilv_world_new failed");
		return 1;
	}
	lilv_world_load_all(world);
	LilvNode *uri = lilv_new_uri(world, path);
	if( !uri ) {
		printf("lv2_gui: lilv_new_uri(%s) failed", path);
		return 1;
	}

	const LilvPlugins *all_plugins = lilv_world_get_all_plugins(world);
	lilv = lilv_plugins_get_by_uri(all_plugins, uri);
	lilv_node_free(uri);
	if( !lilv ) {
		printf("lv2_gui: lilv_plugins_get_by_uriPlugin(%s) failed", path);
		return 1;
	}

	LilvNode *name = lilv_plugin_get_name(lilv);
	const char *nm = lilv_node_as_string(name);
	snprintf(title,sizeof(title),"L2_%s",nm);
	lilv_node_free(name);

	config.init_lv2(lilv);

	lv2_InputPort  = lilv_new_uri(world, LV2_CORE__InputPort);
	lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);

	for( int i=0; i<config.nb_ports; ++i ) {
		const LilvPort *lp = lilv_plugin_get_port_by_index(lilv, i);
		if( lilv_port_is_a(lilv, lp, lv2_InputPort) &&
		    lilv_port_is_a(lilv, lp, lv2_ControlPort) ) {
			config.append(new PluginLV2Client_Opt(&config, i));
		}
	}

	uri_map = new LV2_URI_Map_Feature();
	uri_map->callback_data = (LV2_URI_Map_Callback_Data)this;
	uri_map->uri_to_id = uri_to_id;
	ui_features.append(new Lv2Feature(NS_EXT "uri-map", uri_map));
	map.handle = (void*)&uri_table;
	map.map = uri_table_map;
	ui_features.append(new Lv2Feature(LV2_URID__map, &map));
	unmap.handle = (void*)&uri_table;
	unmap.unmap  = uri_table_unmap;
	ui_features.append(new Lv2Feature(LV2_URID__unmap, &unmap));
	ui_features.append(0);

	int sample_rate = 64; // cant be too low
	inst = lilv_plugin_instantiate(lilv, sample_rate, ui_features);
	if( !inst ) {
		printf("lv2_gui: lilv_plugin_instantiate failed: %s\n", title);
		return 1;
	}

	uis = lilv_plugin_get_uis(lilv);
	if( gtk_type ) {
		LilvNode *gui_type = lilv_new_uri(world, gtk_type);
		LILV_FOREACH(uis, i, uis) {
			const LilvUI *gui = lilv_uis_get(uis, i);
			if( lilv_ui_is_supported(gui, suil_ui_supported, gui_type, &ui_type)) {
				ui = gui;
				break;
			}
		}
		lilv_node_free(gui_type);
	}
	if( !ui )
		ui = lilv_uis_get(uis, lilv_uis_begin(uis));
	if( !ui ) {
		printf("lv2_gui: init_ui failed: %s\n", title);
		return 1;
	}

	lilv_instance_activate(inst);
	return 0;
}

void PluginLV2GUI::update_value(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr)
{
	if( idx >= config.nb_ports ) return;
	for( int i=0, sz=config.size(); i<sz; ++i ) {
		PluginLV2Client_Opt *opt = config[i];
		if( opt->idx == idx ) {
			opt->set_value(*(const float*)bfr);
//printf("set %s = %f\n", opt->get_symbol(), opt->get_value());
			++updates;
			break;
		}
	}
}
void PluginLV2GUI::write_from_ui(void *the, uint32_t idx,
		uint32_t bfrsz, uint32_t typ, const void *bfr)
{
	((PluginLV2GUI*)the)->update_value(idx, bfrsz, typ, bfr);
}


uint32_t PluginLV2GUI::port_index(void* obj, const char* sym)
{
	PluginLV2GUI *the = (PluginLV2GUI*)obj;
	for( int i=0, sz=the->config.size(); i<sz; ++i ) {
		PluginLV2Client_Opt *opt = the->config[i];
		if( !strcmp(sym, opt->sym) ) return opt->idx;
	}
	return UINT32_MAX;
}

void PluginLV2GUI::update_control(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr)
{
	if( !sinst || idx >= config.nb_ports ) return;
	suil_instance_port_event(sinst, idx, bfrsz, typ, bfr);
}


#if 0
void PluginLV2GUI::touch(void *obj, uint32_t pidx, bool grabbed)
{
	PluginLV2GUI* the = (PluginLV2GUI*)obj;
	int idx = pidx;
	if( idx >= the->config.nb_ports ) return;
printf("%s %s(%u)\n", (grabbed? _("press") : _("release")),
  the->config.names[idx], idx);
}
#endif

uint32_t PluginLV2GUI::uri_to_id(LV2_URID_Map_Handle handle, const char *map, const char *uri)
{
	return ((PluginLV2UriTable *)handle)->map(uri);
}

LV2_URID PluginLV2GUI::uri_table_map(LV2_URID_Map_Handle handle, const char *uri)
{
	return ((PluginLV2UriTable *)handle)->map(uri);
}

const char *PluginLV2GUI::uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
{
	return ((PluginLV2UriTable *)handle)->unmap(urid);
}

void PluginLV2GUI::lv2ui_instantiate(void *parent)
{
	if ( !ui_host ) {
		ui_host = suil_host_new(
			PluginLV2GUI::write_from_ui,
			PluginLV2GUI::port_index,
			0, 0);
//		suil_host_set_touch_func(ui_host,
//			PluginLV2GUI::touch);
	}

	ui_features.remove();
	LV2_Handle lilv_handle = lilv_instance_get_handle(inst);
	ui_features.append(new Lv2Feature(NS_EXT "instance-access", lilv_handle));
	const LV2_Descriptor *lilv_desc = lilv_instance_get_descriptor(inst);
	ext_data = new LV2_Extension_Data_Feature();
	ext_data->data_access = lilv_desc->extension_data;
	ui_features.append(new Lv2Feature(LV2_DATA_ACCESS_URI, ext_data));
	ui_features.append(new Lv2Feature(LV2_UI__parent, parent));
	ui_features.append(new Lv2Feature(LV2_UI__idleInterface, 0));
	ui_features.append(0);

	const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(ui));
	char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
	const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(ui));
	char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);
	sinst = suil_instance_new(ui_host, this, gtk_type,
		lilv_node_as_uri(lilv_plugin_get_uri(lilv)),
		lilv_node_as_uri(lilv_ui_get_uri(ui)),
		lilv_node_as_uri(ui_type),
		bundle_path, binary_path, ui_features);

	lilv_free(binary_path);
	lilv_free(bundle_path);
}

bool PluginLV2GUI::lv2ui_resizable()
{
	if( !ui ) return false;
        const LilvNode* s   = lilv_ui_get_uri(ui);
        LilvNode *p   = lilv_new_uri(world, LV2_CORE__optionalFeature);
        LilvNode *fs  = lilv_new_uri(world, LV2_UI__fixedSize);
        LilvNode *nrs = lilv_new_uri(world, LV2_UI__noUserResize);
        LilvNodes *fs_matches = lilv_world_find_nodes(world, s, p, fs);
        LilvNodes *nrs_matches = lilv_world_find_nodes(world, s, p, nrs);
        lilv_nodes_free(nrs_matches);
        lilv_nodes_free(fs_matches);
        lilv_node_free(nrs);
        lilv_node_free(fs);
        lilv_node_free(p);
        return !fs_matches && !nrs_matches;
}

int PluginLV2GUI::update_lv2(float *vals, int force)
{
	int ret = 0;
	float *ctls = (float *)config.ctls;
	for( int i=0; i<config.size(); ++i ) {
		int idx = config[i]->idx;
		float val = vals[idx];
		if( !force && ctls[idx] == val ) continue;
		update_control(idx, sizeof(val), 0, &val);
		++ret;
	}
	for( int i=0; i<config.nb_ports; ++i ) ctls[i] = vals[i];
	return ret;
}

#endif /* HAVE_LV2UI */
