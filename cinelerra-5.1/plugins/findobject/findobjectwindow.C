
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "language.h"
#include "findobject.h"
#include "findobjectwindow.h"
#include "theme.h"


FindObjectWindow::FindObjectWindow(FindObjectMain *plugin)
 : PluginClientWindow(plugin, 340, 660, 340, 660, 0)
{
	this->plugin = plugin;
}

FindObjectWindow::~FindObjectWindow()
{
}

void FindObjectWindow::create_objects()
{
	int x = 10, y = 10, x1 = x, x2 = get_w()/2;
	plugin->load_configuration();

	BC_Title *title;
	add_subwindow(title = new BC_Title(x1, y, _("Algorithm:")));
	add_subwindow(algorithm = new FindObjectAlgorithm(plugin, this,
		x1 + title->get_w() + 10, y));
	algorithm->create_objects();
	y += algorithm->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(use_flann = new FindObjectUseFlann(plugin, this, x, y));
	y += use_flann->get_h() + plugin->get_theme()->widget_border + 20;

	int x0 = x + 200;
	add_subwindow(title = new BC_Title(x, y, _("Output/scene layer:")));
	scene_layer = new FindObjectLayer(plugin, this, x0, y,
		&plugin->config.scene_layer);
	scene_layer->create_objects();
	y += scene_layer->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Object layer:")));
	object_layer = new FindObjectLayer(plugin, this, x0, y,
		&plugin->config.object_layer);
	object_layer->create_objects();
	y += object_layer->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Replacement object layer:")));
	replace_layer = new FindObjectLayer(plugin, this, x0, y,
		&plugin->config.replace_layer);
	replace_layer->create_objects();
	y += replace_layer->get_h() + plugin->get_theme()->widget_border + 10;

	add_subwindow(title = new BC_Title(x+15, y, _("Units: 0 to 100 percent")));
	y += title->get_h();

	int y1 = y;
	add_subwindow(new BC_Title(x1, y + 10, _("Scene X:")));
	y += title->get_h() + 15;
	add_subwindow(scene_x = new FindObjectScanFloat(plugin, this,
		x1, y, &plugin->config.scene_x));
	add_subwindow(scene_x_text = new FindObjectScanFloatText(plugin, this,
		x1 + scene_x->get_w() + 10, y + 10, &plugin->config.scene_x));
	scene_x->center_text = scene_x_text;
	scene_x_text->center = scene_x;

	y += 40;
	add_subwindow(title = new BC_Title(x1, y + 10, _("Scene Y:")));
	y += title->get_h() + 15;
	add_subwindow(scene_y = new FindObjectScanFloat(plugin, this,
		x1, y, &plugin->config.scene_y));
	add_subwindow(scene_y_text = new FindObjectScanFloatText(plugin, this,
		x1 + scene_y->get_w() + 10, y + 10, &plugin->config.scene_y));
	scene_y->center_text = scene_y_text;
	scene_y_text->center = scene_y;

	y += 40;
	add_subwindow(new BC_Title(x1, y + 10, _("Scene W:")));
	y += title->get_h() + 15;
	add_subwindow(scene_w = new FindObjectScanFloat(plugin, this,
		x1, y, &plugin->config.scene_w));
	add_subwindow(scene_w_text = new FindObjectScanFloatText(plugin, this,
		x1 + scene_w->get_w() + 10, y + 10, &plugin->config.scene_w));
	scene_w->center_text = scene_w_text;
	scene_w_text->center = scene_w;

	y += 40;
	add_subwindow(title = new BC_Title(x1, y + 10, _("Scene H:")));
	y += title->get_h() + 15;
	add_subwindow(scene_h = new FindObjectScanFloat(plugin, this,
		x1, y, &plugin->config.scene_h));
	add_subwindow(scene_h_text = new FindObjectScanFloatText(plugin, this,
		x1 + scene_h->get_w() + 10, y + 10,
		&plugin->config.scene_h));
	scene_h->center_text = scene_h_text;
	scene_h_text->center = scene_h;

	y = y1;
	add_subwindow(new BC_Title(x2, y + 10, _("Object X:")));
	y += title->get_h() + 15;
	add_subwindow(object_x = new FindObjectScanFloat(plugin, this,
		x2, y, &plugin->config.object_x));
	add_subwindow(object_x_text = new FindObjectScanFloatText(plugin, this,
		x2 + object_x->get_w() + 10, y + 10, &plugin->config.object_x));
	object_x->center_text = object_x_text;
	object_x_text->center = object_x;

	y += 40;
	add_subwindow(title = new BC_Title(x2, y + 10, _("Object Y:")));
	y += title->get_h() + 15;
	add_subwindow(object_y = new FindObjectScanFloat(plugin, this,
		x2, y, &plugin->config.object_y));
	add_subwindow(object_y_text = new FindObjectScanFloatText(plugin, this,
		x2 + object_y->get_w() + 10, y + 10, &plugin->config.object_y));
	object_y->center_text = object_y_text;
	object_y_text->center = object_y;

	y += 40;
	add_subwindow(new BC_Title(x2, y + 10, _("Object W:")));
	y += title->get_h() + 15;
	add_subwindow(object_w = new FindObjectScanFloat(plugin, this,
		x2, y, &plugin->config.object_w));
	add_subwindow(object_w_text = new FindObjectScanFloatText(plugin, this,
		x2 + object_w->get_w() + 10, y + 10, &plugin->config.object_w));
	object_w->center_text = object_w_text;
	object_w_text->center = object_w;

	y += 40;
	add_subwindow(title = new BC_Title(x2, y + 10, _("Object H:")));
	y += title->get_h() + 15;
	add_subwindow(object_h = new FindObjectScanFloat(plugin, this,
		x2, y, &plugin->config.object_h));
	add_subwindow(object_h_text = new FindObjectScanFloatText(plugin, this,
		x2 + object_h->get_w() + 10, y + 10,
		&plugin->config.object_h));
	object_h->center_text = object_h_text;
	object_h_text->center = object_h;

	y += 40 + 15;
	add_subwindow(draw_keypoints = new FindObjectDrawKeypoints(plugin, this, x, y));
	y += draw_keypoints->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(draw_border = new FindObjectDrawBorder(plugin, this, x, y));
	y += draw_border->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(replace_object = new FindObjectReplace(plugin, this, x, y));
	y += replace_object->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(draw_object_border = new FindObjectDrawObjectBorder(plugin, this, x, y));
	y += draw_object_border->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y + 10, _("Object blend amount:")));
	add_subwindow(blend = new FindObjectBlend(plugin,
		x + title->get_w() + plugin->get_theme()->widget_border, y,
		&plugin->config.blend));
	y += blend->get_h();

	show_window(1);
}


FindObjectScanFloat::FindObjectScanFloat(FindObjectMain *plugin, FindObjectWindow *gui,
	int x, int y, float *value)
 : BC_FPot(x, y, *value, (float)0, (float)100)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
	this->center_text = 0;
	set_precision(0.1);
}

int FindObjectScanFloat::handle_event()
{
	*value = get_value();
	center_text->update(*value);
	plugin->send_configure_change();
	return 1;
}


FindObjectScanFloatText::FindObjectScanFloatText(FindObjectMain *plugin, FindObjectWindow *gui,
	int x, int y, float *value)
 : BC_TextBox(x, y, 75, 1, *value)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
	this->center = 0;
	set_precision(1);
}

int FindObjectScanFloatText::handle_event()
{
	*value = atof(get_text());
	center->update(*value);
	plugin->send_configure_change();
	return 1;
}


FindObjectDrawBorder::FindObjectDrawBorder(FindObjectMain *plugin, FindObjectWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_border, _("Draw border"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectDrawBorder::handle_event()
{
	plugin->config.draw_border = get_value();
	plugin->send_configure_change();
	return 1;
}


FindObjectDrawKeypoints::FindObjectDrawKeypoints(FindObjectMain *plugin, FindObjectWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_keypoints, _("Draw keypoints"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectDrawKeypoints::handle_event()
{
	plugin->config.draw_keypoints = get_value();
	plugin->send_configure_change();
	return 1;
}


FindObjectReplace::FindObjectReplace(FindObjectMain *plugin, FindObjectWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.replace_object, _("Replace object"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectReplace::handle_event()
{
	plugin->config.replace_object = get_value();
	plugin->send_configure_change();
	return 1;
}


FindObjectDrawObjectBorder::FindObjectDrawObjectBorder(FindObjectMain *plugin, FindObjectWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.draw_object_border, _("Draw object border"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectDrawObjectBorder::handle_event()
{
	plugin->config.draw_object_border = get_value();
	plugin->send_configure_change();
	return 1;
}


FindObjectAlgorithm::FindObjectAlgorithm(FindObjectMain *plugin, FindObjectWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, calculate_w(gui), to_text(plugin->config.algorithm))
{
	this->plugin = plugin;
	this->gui = gui;
}

int FindObjectAlgorithm::handle_event()
{
	plugin->config.algorithm = from_text(get_text());
	plugin->send_configure_change();
	return 1;
}

void FindObjectAlgorithm::create_objects()
{
	add_item(new BC_MenuItem(to_text(NO_ALGORITHM)));
#ifdef _SIFT
	add_item(new BC_MenuItem(to_text(ALGORITHM_SIFT)));
#endif
#ifdef _SURF
	add_item(new BC_MenuItem(to_text(ALGORITHM_SURF)));
#endif
#ifdef _ORB
	add_item(new BC_MenuItem(to_text(ALGORITHM_ORB)));
#endif
#ifdef _AKAZE
	add_item(new BC_MenuItem(to_text(ALGORITHM_AKAZE)));
#endif
#ifdef _BRISK
	add_item(new BC_MenuItem(to_text(ALGORITHM_BRISK)));
#endif
}

int FindObjectAlgorithm::from_text(char *text)
{
#ifdef _SIFT
	if(!strcmp(text, _("SIFT"))) return ALGORITHM_SIFT;
#endif
#ifdef _SURF
	if(!strcmp(text, _("SURF"))) return ALGORITHM_SURF;
#endif
#ifdef _ORB
	if(!strcmp(text, _("ORB"))) return ALGORITHM_ORB;
#endif
#ifdef _AKAZE
	if(!strcmp(text, _("AKAZE"))) return ALGORITHM_AKAZE;
#endif
#ifdef _BRISK
	if(!strcmp(text, _("BRISK"))) return ALGORITHM_BRISK;
#endif
	return NO_ALGORITHM;
}

char* FindObjectAlgorithm::to_text(int mode)
{
	switch( mode ) {
#ifdef _SIFT
	case ALGORITHM_SIFT:	return _("SIFT");
#endif
#ifdef _SURF
	case ALGORITHM_SURF:	return _("SURF");
#endif
#ifdef _ORB
	case ALGORITHM_ORB:	return _("ORB");
#endif
#ifdef _AKAZE
	case ALGORITHM_AKAZE:	return _("AKAZE");
#endif
#ifdef _BRISK
	case ALGORITHM_BRISK:	return _("BRISK");
#endif
	}
	return _("Don't Calculate");
}

int FindObjectAlgorithm::calculate_w(FindObjectWindow *gui)
{
	int result = 0;
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(NO_ALGORITHM)));
#ifdef _SIFT
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_SIFT)));
#endif
#ifdef _SURF
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_SURF)));
#endif
#ifdef _ORB
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_ORB)));
#endif
#ifdef _AKAZE
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_AKAZE)));
#endif
#ifdef _BRISK
	result = MAX(result, gui->get_text_width(MEDIUMFONT, to_text(ALGORITHM_BRISK)));
#endif
	return result + 50;
}


FindObjectUseFlann::FindObjectUseFlann(FindObjectMain *plugin, FindObjectWindow *gui,
	int x, int y)
 : BC_CheckBox(x, y, plugin->config.use_flann, _("Use FLANN"))
{
	this->gui = gui;
	this->plugin = plugin;
}

int FindObjectUseFlann::handle_event()
{
	plugin->config.use_flann = get_value();
	plugin->send_configure_change();
	return 1;
}


FindObjectLayer::FindObjectLayer(FindObjectMain *plugin, FindObjectWindow *gui,
	int x, int y, int *value)
 : BC_TumbleTextBox(gui, *value, MIN_LAYER, MAX_LAYER, x, y, calculate_w(gui))
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
}

int FindObjectLayer::handle_event()
{
	*value = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}

int FindObjectLayer::calculate_w(FindObjectWindow *gui)
{
	int result = 0;
	result = gui->get_text_width(MEDIUMFONT, "000");
	return result + 50;
}


FindObjectBlend::FindObjectBlend(FindObjectMain *plugin,
	int x,
	int y,
	int *value)
 : BC_IPot(x,
		y,
		(int64_t)*value,
		(int64_t)MIN_BLEND,
		(int64_t)MAX_BLEND)
{
	this->plugin = plugin;
	this->value = value;
}

int FindObjectBlend::handle_event()
{
	*value = (int)get_value();
	plugin->send_configure_change();
	return 1;
}

