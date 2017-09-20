#ifndef __PROXY_H__
#define __PROXY_H__

/*
 * CINELERRA
 * Copyright (C) 2015 Adam Williams <broadcast at earthling dot net>
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

// functions for handling proxies

#include "arraylist.h"
#include "asset.h"
#include "bcdialog.h"
#include "formattools.inc"
#include "loadbalance.h"
#include "mutex.inc"
#include "mwindow.inc"

class ProxyThread;
class ProxyWindow;

#define MAX_SIZES 16

class ProxyMenuItem : public BC_MenuItem
{
public:
	ProxyMenuItem(MWindow *mwindow);

	int handle_event();
	void create_objects();

	MWindow *mwindow;
	ProxyThread *thread;
};

class FromProxyMenuItem : public BC_MenuItem
{
public:
	FromProxyMenuItem(MWindow *mwindow);

	int handle_event();
	MWindow *mwindow;
};

class ProxyThread : public BC_DialogThread
{
public:
	ProxyThread(MWindow *mwindow);
	~ProxyThread();
	BC_Window* new_gui();
	void handle_close_event(int result);
	static void to_proxy_path(char *new_path, Asset *asset, int scale);
	static void from_proxy_path(char *new_path, Asset *asset, int scale);
	void from_proxy();
	void to_proxy();
// increment the frame count by 1
	void update_progress();
// if user canceled progress bar
	int is_canceled();
// calculate possible sizes based on the original size
	void calculate_sizes();
	void scale_to_text(char *string, int scale);

	MWindow *mwindow;
	ProxyWindow *gui;
	MainProgressBar *progress;
	Mutex *counter_lock;
	Asset *asset;
	int new_scale;
	int orig_scale;
	int total_rendered;
	int failed;
	int use_scaler;
	char *size_text[MAX_SIZES];
	int size_factors[MAX_SIZES];
	int total_sizes;
};

class ProxyUseScaler : public BC_CheckBox
{
public:
	ProxyUseScaler(MWindow *mwindow, ProxyWindow *pwindow,
		int x, int y);
	void update();
	int handle_event();
	MWindow *mwindow;
	ProxyWindow *pwindow;
};

class ProxyFormatTools : public FormatTools
{
public:
	ProxyFormatTools(MWindow *mwindow, ProxyWindow *window, Asset *asset);

	void update_format();
	ProxyWindow *pwindow;
};

class ProxyMenu : public BC_PopupMenu
{
public:
	ProxyMenu(MWindow *mwindow, ProxyWindow *pwindow,
		int x, int y, int w, const char *text);
	void update_sizes();
	int handle_event();
	MWindow *mwindow;
	ProxyWindow *pwindow;
};


class ProxyTumbler : public BC_Tumbler
{
public:
	ProxyTumbler(MWindow *mwindow, ProxyWindow *pwindow, int x, int y);

	int handle_up_event();
	int handle_down_event();
	
	ProxyWindow *pwindow;
	MWindow *mwindow;
};


class ProxyWindow : public BC_Window
{
public:
	ProxyWindow(MWindow *mwindow, ProxyThread *thread,
		int x, int y);
	~ProxyWindow();

	void create_objects();
	void update();

	MWindow *mwindow;
	ProxyThread *thread;
	FormatTools *format_tools;
	BC_Title *new_dimensions;
	ProxyMenu *scale_factor;
	ProxyUseScaler *use_scaler;
};

class ProxyFarm;

class ProxyPackage : public LoadPackage
{
public:
	ProxyPackage();
	Asset *orig_asset;
	Asset *proxy_asset;
};

class ProxyClient : public LoadClient
{
public:
	ProxyClient(MWindow *mwindow, ProxyThread *thread,
		ProxyFarm *server);
	void process_package(LoadPackage *package);

	MWindow *mwindow;
	ProxyThread *thread;
};


class ProxyFarm : public LoadServer
{
public:
	ProxyFarm(MWindow *mwindow, ProxyThread *thread, 
		ArrayList<Asset*> *proxy_assets, ArrayList<Asset*> *orig_assets);
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	MWindow *mwindow;
	ProxyThread *thread;
	ArrayList<Asset*> *proxy_assets;
	ArrayList<Asset*> *orig_assets;
};

#endif
