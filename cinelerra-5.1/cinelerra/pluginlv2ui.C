#ifdef HAVE_LV2UI

// shared between parent/child fork
#include "language.h"
#include "pluginlv2ui.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <ctype.h>
#include <string.h>

int PluginLV2UI::init_ui(const char *path, int sample_rate)
{
	if( load_lv2(path, title) ) return 1;
	if( init_lv2(config, sample_rate) ) return 1;

	lilv_uis = lilv_plugin_get_uis(lilv);
	if( !lilv_uis ) {
		printf("lv2: lilv_plugin_get_uis(%s) failed\n", path);
		return 1;
	}

	if( gtk_type ) {
		LilvNode *gui_type = lilv_new_uri(world, gtk_type);
		LILV_FOREACH(uis, i, lilv_uis) {
			const LilvUI *ui = lilv_uis_get(lilv_uis, i);
			if( lilv_ui_is_supported(ui, suil_ui_supported, gui_type, &lilv_type)) {
				lilv_ui = ui;
				break;
			}
		}
		lilv_node_free(gui_type);
	}
	if( !lilv_ui )
		lilv_ui = lilv_uis_get(lilv_uis, lilv_uis_begin(lilv_uis));
	if( !lilv_ui ) {
		printf("lv2_gui: init_ui failed: %s\n", title);
		return 1;
	}

	lilv_instance_activate(inst);
	return 0;
}

void PluginLV2UI::update_value(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr)
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
void PluginLV2UI::write_from_ui(void *the, uint32_t idx,
		uint32_t bfrsz, uint32_t typ, const void *bfr)
{
	((PluginLV2UI*)the)->update_value(idx, bfrsz, typ, bfr);
}

uint32_t PluginLV2UI::port_index(void* obj, const char* sym)
{
	PluginLV2UI *the = (PluginLV2UI*)obj;
	for( int i=0, sz=the->config.size(); i<sz; ++i ) {
		PluginLV2Client_Opt *opt = the->config[i];
		if( !strcmp(sym, opt->sym) ) return opt->idx;
	}
	return UINT32_MAX;
}

void PluginLV2UI::update_control(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr)
{
	if( !sinst || idx >= config.nb_ports ) return;
	suil_instance_port_event(sinst, idx, bfrsz, typ, bfr);
}


#if 0
void PluginLV2UI::touch(void *obj, uint32_t pidx, bool grabbed)
{
	PluginLV2UI* the = (PluginLV2GUI*)obj;
	int idx = pidx;
	if( idx >= the->config.nb_ports ) return;
printf("%s %s(%u)\n", (grabbed? _("press") : _("release")),
  the->config.names[idx], idx);
}
#endif

uint32_t PluginLV2UI::uri_to_id(LV2_URID_Map_Handle handle, const char *map, const char *uri)
{
	return ((PluginLV2UriTable *)handle)->map(uri);
}

LV2_URID PluginLV2UI::uri_table_map(LV2_URID_Map_Handle handle, const char *uri)
{
	return ((PluginLV2UriTable *)handle)->map(uri);
}

const char *PluginLV2UI::uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
{
	return ((PluginLV2UriTable *)handle)->unmap(urid);
}

void PluginLV2UI::lv2ui_instantiate(void *parent)
{
	if ( !ui_host ) {
		ui_host = suil_host_new(
			PluginLV2UI::write_from_ui,
			PluginLV2UI::port_index,
			0, 0);
//		suil_host_set_touch_func(ui_host,
//			PluginLV2GUI::touch);
	}

	features.remove();  // remove terminating zero
	ui_features = features.size();
	LV2_Handle lilv_handle = lilv_instance_get_handle(inst);
	features.append(new Lv2Feature(NS_EXT "instance-access", lilv_handle));
	const LV2_Descriptor *lilv_desc = lilv_instance_get_descriptor(inst);
	ext_data = new LV2_Extension_Data_Feature();
	ext_data->data_access = lilv_desc->extension_data;
	features.append(new Lv2Feature(LV2_DATA_ACCESS_URI, ext_data));
	features.append(new Lv2Feature(LV2_UI__parent, parent));
	features.append(new Lv2Feature(LV2_UI__idleInterface, 0));
	features.append(0); // add new terminating zero

	const char* bundle_uri  = lilv_node_as_uri(lilv_ui_get_bundle_uri(lilv_ui));
	char*       bundle_path = lilv_file_uri_parse(bundle_uri, NULL);
	const char* binary_uri  = lilv_node_as_uri(lilv_ui_get_binary_uri(lilv_ui));
	char*       binary_path = lilv_file_uri_parse(binary_uri, NULL);
	sinst = suil_instance_new(ui_host, this, gtk_type,
		lilv_node_as_uri(lilv_plugin_get_uri(lilv)),
		lilv_node_as_uri(lilv_ui_get_uri(lilv_ui)),
		lilv_node_as_uri(lilv_type),
		bundle_path, binary_path, features);

	lilv_free(binary_path);
	lilv_free(bundle_path);
}

bool PluginLV2UI::lv2ui_resizable()
{
	if( !lilv_ui ) return false;
	const LilvNode* s   = lilv_ui_get_uri(lilv_ui);
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

int PluginLV2UI::update_lv2(float *vals, int force)
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

static void lilv_destroy(GtkWidget* widget, gpointer data)
{
	PluginLV2UI *the = (PluginLV2UI*)data;
	the->hidden = 1;
	the->top_level = 0;
	++the->updates;
}

void PluginLV2UI::start_gui()
{
	top_level = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(top_level, "destroy", G_CALLBACK(lilv_destroy), this);
	gtk_window_set_title(GTK_WINDOW(top_level), title);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_set_role(GTK_WINDOW(top_level), "plugin_ui");
	gtk_container_add(GTK_CONTAINER(top_level), vbox);

	GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
	gtk_widget_show(alignment);
	lv2ui_instantiate(alignment);
	GtkWidget* widget = (GtkWidget*)suil_instance_get_widget(sinst);
	gtk_container_add(GTK_CONTAINER(alignment), widget);
	gtk_window_set_resizable(GTK_WINDOW(top_level), lv2ui_resizable());
	gtk_widget_show_all(vbox);
	gtk_widget_grab_focus(widget);
	float *ctls = (float *)config.ctls;
	update_lv2(ctls, 1);
	connect_ports(config, TYP_CONTROL);
	lilv_instance_run(inst, 0);
	gtk_window_present(GTK_WINDOW(top_level));
}


void PluginLV2UI::host_update(PluginLV2ChildUI *child)
{
//printf("update\n");
	host_updates = updates;
	if( !child ) return;
	if( host_hidden != hidden ) {
		host_hidden = hidden;
		if( hidden ) reset_gui();
		child->send_parent(hidden ? LV2_HIDE : LV2_SHOW, 0, 0);
	}
	if( running < 0 ) { running = 1;  return; }
	child->send_parent(LV2_UPDATE, config.ctls, sizeof(float)*config.nb_ports);
}

void PluginLV2UI::reset_gui()
{
	if( sinst )     { suil_instance_free(sinst);  sinst = 0; }
	if( ui_host )   { suil_host_free(ui_host);    ui_host = 0; }
	if( top_level ) { gtk_widget_destroy(top_level); top_level = 0; }

	while( features.size() > ui_features ) features.remove_object();
	features.append(0);
	hidden = 1;
}


// child main
int PluginLV2UI::run_ui(PluginLV2ChildUI *child)
{
	running = 1;
	while( !done ) {
		if( gtk_events_pending() ) {
			gtk_main_iteration();
			continue;
		}
		if( running && host_updates != updates )
			host_update(child);
		if( redraw ) {
			redraw = 0;
			update_lv2(config.ctls, 1);
		}
		if( !child ) usleep(10000);
		else if( child->child_iteration() < 0 )
			done = 1;
	}
	running = 0;
	return 0;
}

void PluginLV2UI::run_buffer(int shmid)
{
	if( !shm_buffer(shmid) ) return;
	map_buffer();
	int samples = shm_bfr->samples;
	connect_ports(config);
	lilv_instance_run(inst, samples);
	shm_bfr->done = 1;
}

int PluginLV2ChildUI::handle_child()
{
	switch( child_token ) {
	case LV2_OPEN: {
		open_bfr_t *open_bfr = (open_bfr_t *)child_data;
		if( init_ui(open_bfr->path, open_bfr->sample_rate) ) exit(1);
		break; }
	case LV2_LOAD: {
		float *ctls = (float *)child_data;
		update_lv2(ctls, 1);
		break; }
	case LV2_UPDATE: {
		float *ctls = (float *)child_data;
		if( update_lv2(ctls, 0) > 0 )
			++updates;
		break; }
	case LV2_SHOW: {
		start_gui();
		hidden = 0;  ++updates;
		break; }
	case LV2_HIDE: {
		hidden = 1;  ++updates;
		break; }
	case LV2_SET: {
		control_bfr_t *ctl_bfr = (control_bfr_t *)child_data;
		config.ctls[ctl_bfr->idx] = ctl_bfr->value;
		redraw = 1;
		break; }
	case LV2_SHMID: {
		int shmid = *(int *)child_data;
		run_buffer(shmid);
		send_parent(LV2_SHMID, 0, 0);
		break; }
	case EXIT_CODE:
		return -1;
	}
	return 1;
}

#endif /* HAVE_LV2UI */
