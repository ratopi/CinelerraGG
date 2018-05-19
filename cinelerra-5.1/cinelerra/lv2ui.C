#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "bcsignals.h"
#include "pluginlv2client.h"
#include "pluginlv2gui.h"

static void lilv_destroy(GtkWidget* widget, gpointer data)
{
	PluginLV2GUI *the = (PluginLV2GUI*)data;
	the->done = 1;
}

void PluginLV2GUI::start()
{
	GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(window, "destroy", G_CALLBACK(lilv_destroy), this);
	gtk_window_set_title(GTK_WINDOW(window), title);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_set_role(GTK_WINDOW(window), "plugin_ui");
	gtk_container_add(GTK_CONTAINER(window), vbox);

	GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);
	gtk_widget_show(alignment);
	lv2ui_instantiate(alignment);
	GtkWidget* widget = (GtkWidget*)suil_instance_get_widget(sinst);
	gtk_container_add(GTK_CONTAINER(alignment), widget);
	gtk_window_set_resizable(GTK_WINDOW(window), lv2ui_resizable());
	gtk_widget_show_all(vbox);
	gtk_widget_grab_focus(widget);

	gtk_window_present(GTK_WINDOW(window));
	running = -1;
}

void PluginLV2GUI::stop()
{
	running = 0;
}

void PluginLV2GUI::host_update(PluginLV2ChildGUI *child)
{
//printf("update\n");
	last = updates;
	if( !child ) return;
// ignore reset update
	if( child->lv2_gui->running < 0 ) { child->lv2_gui->running = 1;  return; }
	child->send_parent(LV2_UPDATE, config.ctls, sizeof(float)*config.nb_ports);
}

void PluginLV2GUI::run_gui(PluginLV2ChildGUI *child)
{
	while( !done ) {
		if( gtk_events_pending() ) {
			gtk_main_iteration();
			continue;
		}
		if( running && updates != last )
			host_update(child);
		if( redraw ) {
			redraw = 0;
			update_lv2(config.ctls, 1);
		}
		if( !child ) usleep(10000);
		else if( child->child_iteration() < 0 )
			done = 1;
	}
}

int PluginLV2ChildGUI::handle_child()
{
	if( !lv2_gui ) return 0;

	switch( child_token ) {
	case LV2_OPEN: {
		char *path = (char *)child_data;
		if( lv2_gui->init_gui(path) ) exit(1);
		break; }
	case LV2_LOAD: {
		lv2_gui->update_lv2((float*)child_data, 1);
		break; }
	case LV2_UPDATE: {
		lv2_gui->update_lv2((float*)child_data, 0);
		break; }
	case LV2_START: {
		lv2_gui->start();
		break; }
	case LV2_SET: {
		if( !lv2_gui ) break;
		control_t *bfr = (control_t *)child_data;
		lv2_gui->config.ctls[bfr->idx] = bfr->value;
		lv2_gui->redraw = 1;
		break; }
	case EXIT_CODE:
		return -1;
	default:
		return 0;
	}
	return 1;
}

int PluginLV2GUI::run(int ac, char **av)
{
	if( ac < 3 ) {
		if( init_gui(av[1]) ) {
			fprintf(stderr," init_ui failed\n");
			return 1;
		}
		start();
		run_gui();
		stop();
	}
	else {
		PluginLV2ChildGUI child;
		child.lv2_gui = this;
		child.child_fd = atoi(av[1]);
		child.parent_fd = atoi(av[2]);
		run_gui(&child);
		stop();
		child.lv2_gui = 0;
	}
	return 0;
}

int main(int ac, char **av)
{
	BC_Signals signals;
	if( getenv("BC_TRAP_LV2_SEGV") ) {
		signals.initialize("/tmp/lv2ui_%d.dmp");
	        BC_Signals::set_catch_segv(1);
	}
	gtk_set_locale();
	gtk_init(&ac, &av);
// to grab this task in the debugger
//static int zbug = 1;  volatile int bug = zbug;
//while( bug ) usleep(10000);
	return PluginLV2GUI().run(ac, av);
}

