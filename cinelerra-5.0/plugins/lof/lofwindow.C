#include "lofwindow.h"
#include "language.h"

LofWindow::LofWindow(LofEffect *plugin)
 : PluginClientWindow(plugin, 260, 160, 260, 160, 0)
{
	this->plugin = plugin;
}

void LofWindow::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title = new BC_Title(x, y, _("Show last good output frame"));
	add_subwindow(title);  y += title->get_h() + 5;
	title = new BC_Title(x+20, y, _("(you should fix the input)"));
	add_subwindow(title);  y += title->get_h() + 20;
        add_tool(errfrms = new LofToggle(this, &plugin->config.errs, x, y, _("errant frames")));
	y += errfrms->get_h() + 5;
        add_tool(misfrms = new LofToggle(this, &plugin->config.miss, x, y, _("missed frames")));
	y += misfrms->get_h() + 5;
        add_tool(mrkfrms = new LofToggle(this, &plugin->config.mark, x, y, _("mark fixed frames")));
	show_window();
	flush();
}

void LofWindow::update()
{
	errfrms->update(plugin->config.errs);
	misfrms->update(plugin->config.miss);
	mrkfrms->update(plugin->config.mark);
}

LofToggle::LofToggle(LofWindow *lofwin, int *output, int x, int y, const char *lbl)
 : BC_CheckBox(x, y, *output, lbl)
{
        this->lofwin = lofwin;
        this->output = output;
}

LofToggle::~LofToggle()
{
}

int LofToggle::handle_event()
{
        *output = get_value();
        lofwin->plugin->send_configure_change();
	return 1;
}

