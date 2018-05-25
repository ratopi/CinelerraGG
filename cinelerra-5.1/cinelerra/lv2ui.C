#include <stdio.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "bcsignals.h"
#include "pluginlv2client.h"
#include "pluginlv2ui.h"

int PluginLV2UI::run(int ac, char **av)
{
	int sample_rate = 48000;
	if( ac > 2 ) sample_rate = atoi(av[2]);
	if( init_ui(av[1], sample_rate) ) {
		fprintf(stderr," init_ui failed\n");
		return 1;
	}
	start_gui();
	return run_ui();
}

int PluginLV2ChildUI::run(int ac, char **av)
{
	signal(SIGINT, SIG_IGN);
	ForkBase::child_fd = atoi(av[1]);
	ForkBase::parent_fd = atoi(av[2]);
	ForkBase::ppid = atoi(av[3]);
	return run_ui(this);
}


int main(int ac, char **av)
{
// to grab this task in the debugger
const char *cp = getenv("BUG");
static int zbug = !cp ? 0 : atoi(cp);  volatile int bug = zbug;
while( bug ) usleep(10000);
	BC_Signals signals;
	if( getenv("BC_TRAP_LV2_SEGV") ) {
		signals.initialize("/tmp/lv2ui_%d.dmp");
		BC_Signals::set_catch_segv(1);
	}
	gtk_set_locale();
	gtk_init(&ac, &av);
	return ac < 3 ?
		PluginLV2UI().run(ac, av) :
		PluginLV2ChildUI().run(ac, av);
}

