#ifndef __LOFWINDOW_H__
#define __LOFWINDOW_H__

#include "lof.h"
#include "pluginvclient.h"

class LofToggle;
class LofWindow;

class LofToggle : public BC_CheckBox
{
public:
        LofToggle(LofWindow *lofwin, int *output, int x, int y, const char *lbl);
        ~LofToggle();
        int handle_event();

	LofWindow *lofwin;
        int *output;
};

class LofWindow : public PluginClientWindow {
public:
	LofWindow(LofEffect *plugin);
	void create_objects();
	void update();
	LofEffect *plugin;

	LofToggle *errfrms;
	LofToggle *misfrms;
	LofToggle *mrkfrms;
};


#endif
