
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

#include "assets.h"
#include "bcsignals.h"
#include "clip.h"
#include "confirmsave.h"
#include "cstrdup.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "formattools.h"
#include "language.h"
#include "mainprogress.h"
#include "mainundo.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "overlayframe.h"
#include "preferences.h"
#include "proxy.h"
#include "theme.h"

#define WIDTH 400
#define HEIGHT 320
#define MAX_SCALE 16

ProxyMenuItem::ProxyMenuItem(MWindow *mwindow)
 : BC_MenuItem(_("Proxy settings..."),  _("Alt-P"), 'p')
{
	this->mwindow = mwindow;
	set_alt();
}

void ProxyMenuItem::create_objects()
{
	thread = new ProxyThread(mwindow);
}

int ProxyMenuItem::handle_event()
{
	mwindow->gui->unlock_window();
	thread->start();
	mwindow->gui->lock_window("ProxyMenuItem::handle_event");

	return 1;
}

ProxyThread::ProxyThread(MWindow *mwindow)
{
	this->mwindow = mwindow;
	gui = 0;
	asset = new Asset;
	progress = 0;
	counter_lock = new Mutex("ProxyThread::counter_lock");
	bzero(size_text, sizeof(char*) * MAX_SIZES);
	bzero(size_factors, sizeof(int) * MAX_SIZES);
	total_sizes = 0;
}
ProxyThread::~ProxyThread()
{
	for( int i=0; i<MAX_SIZES; ++i ) delete [] size_text[i];
	asset->remove_user();
}

BC_Window* ProxyThread::new_gui()
{
	asset->format = FILE_FFMPEG;
	asset->load_defaults(mwindow->defaults, "PROXY_", 1, 1, 0, 0, 0);
	mwindow->gui->lock_window("ProxyThread::new_gui");
	int x = mwindow->gui->get_abs_cursor_x(0) - WIDTH / 2;
	int y = mwindow->gui->get_abs_cursor_y(0) - HEIGHT / 2;

	gui = new ProxyWindow(mwindow, this, x, y);
	gui->create_objects();
	mwindow->gui->unlock_window();
	return gui;
}

void ProxyThread::scale_to_text(char *string, int scale)
{
	strcpy(string, size_text[0]);
	for( int i = 0; i < total_sizes; i++ ) {
		if( scale == size_factors[i] ) {
			strcpy(string, size_text[i]);
			break;
		}
	}
}


void ProxyThread::calculate_sizes()
{
	for( int i=1; i<total_sizes; ++i ) {
		delete [] size_text[i];
		size_text[i] = 0;
		size_factors[i] = 0;
	}
	total_sizes = 1;

	int orig_w = mwindow->edl->session->output_w * orig_scale;
	int orig_h = mwindow->edl->session->output_h * orig_scale;

	if( !use_scaler ) {
// w,h should stay even for yuv
		int ow = orig_w, oh = orig_h;
		if( BC_CModels::is_yuv(mwindow->edl->session->color_model) ) {
			ow /= 2;  oh /= 2;
		}
		for( int i=2; i<MAX_SCALE; ++i ) {
			if( (ow % i) != 0 ) continue;
			if( (oh % i) != 0 ) continue;
			size_factors[total_sizes++] = i;
		}
	}
	else {
		size_factors[total_sizes++] = 2;   size_factors[total_sizes++] = 3;
		size_factors[total_sizes++] = 8;   size_factors[total_sizes++] = 12;
		size_factors[total_sizes++] = 16;  size_factors[total_sizes++] = 24;
		size_factors[total_sizes++] = 32;
	}
	for( int i=1; i<total_sizes; ++i ) {
		char string[BCTEXTLEN];
		sprintf(string, "1/%d", size_factors[i]);
		size_text[i] = cstrdup(string);
	}
}

void ProxyThread::handle_close_event(int result)
{
	asset->save_defaults(mwindow->defaults, "PROXY_", 1, 1, 0, 0, 0); 

	if( !result )
		to_proxy();
}

void ProxyThread::to_proxy()
{
// test for new files
	ArrayList<char *> confirm_paths;
	confirm_paths.set_array_delete();
// all proxy assets
	ArrayList<Indexable*> proxy_assets;
// assets which must be created
	ArrayList<Asset*> needed_assets;
// original assets
	ArrayList<Indexable*> orig_assets;
// original assets which match the needed_assets
	ArrayList<Asset*> needed_orig_assets;
	Asset *proxy_asset = 0;
	Asset *orig_asset = 0;

	mwindow->edl->Garbage::add_user();
	mwindow->save_backup();
	mwindow->undo->update_undo_before(_("proxy"), this);

	EDL *&edl = mwindow->edl;
	EDLSession *&session = edl->session;
	Assets *&assets = edl->assets;

// revert project to original size from current size
// remove all session proxy assets at the at the current proxy_scale
	if( session->proxy_scale > 1 ) {
		orig_asset = assets->first;
		for( ; orig_asset; orig_asset=orig_asset->next ) {
			char new_path[BCTEXTLEN];
			to_proxy_path(new_path, orig_asset, session->proxy_scale);
			proxy_asset = assets->get_asset(new_path);
			if( !proxy_asset ) continue;
// test if proxy asset was already added to proxy_assets
			int got_it = 0;
			for( int i = 0; !got_it && i<proxy_assets.size(); ++i )
				got_it = !strcmp(proxy_assets[i]->path, new_path);
			if( got_it ) continue;
// add pointer to existing EDL asset if it exists
// EDL won't delete it unless it's the same pointer.
			proxy_assets.append(proxy_asset);
			proxy_asset->Garbage::add_user();
			orig_assets.append(orig_asset);
			orig_asset->Garbage::add_user();
		}

// convert from the proxy assets to the original assets
		mwindow->set_proxy(0, 1, &proxy_assets, &orig_assets);

// remove the proxy assets
		mwindow->remove_assets_from_project(0, 0, &proxy_assets, NULL);
		for( int i=0; i<proxy_assets.size(); ++i )
			proxy_assets.get(i)->Garbage::remove_user();
		proxy_assets.remove_all();
		for( int i = 0; i < orig_assets.size(); i++ )
			orig_assets.get(i)->Garbage::remove_user();
		orig_assets.remove_all();
	}

// convert to new size if not original size
	if( new_scale != 1 ) {
		orig_asset = assets->first;
		for( ; orig_asset; orig_asset=orig_asset->next ) {
			if( !orig_asset->video_data ) continue;
			char new_path[BCTEXTLEN];
			to_proxy_path(new_path, orig_asset, new_scale);
// add to proxy_assets & orig_assets if it isn't already there.
			int got_it = 0;
			for( int i = 0; !got_it && i<proxy_assets.size(); ++i )
				got_it = !strcmp(proxy_assets[i]->path, new_path);
			if( !got_it ) {
				proxy_asset = new Asset;
// new compression parameters
				proxy_asset->copy_format(asset, 0);
				proxy_asset->update_path(new_path);
				proxy_asset->audio_data = 0;
				proxy_asset->video_data = 1;
				proxy_asset->layers = 1;
				proxy_asset->width = orig_asset->width / new_scale;
				if( proxy_asset->width & 1 ) ++proxy_asset->width;
				proxy_asset->actual_width = proxy_asset->width;
				proxy_asset->height = orig_asset->height / new_scale;
				if( proxy_asset->height & 1 ) ++proxy_asset->height;
				proxy_asset->actual_height = proxy_asset->height;
				proxy_asset->frame_rate = orig_asset->frame_rate;
				proxy_asset->video_length = orig_asset->video_length;
				proxy_assets.append(proxy_asset);
				orig_asset->add_user();
				orig_assets.append(orig_asset);
			}

// test if proxy file exists.
			int exists = 0;
			FILE *fd = fopen(new_path, "r");
			if( fd ) {
				exists = 1;
				fclose(fd);
				FileSystem fs;
// got it if proxy file is newer than original.
				got_it = fs.get_date(new_path) > fs.get_date(asset->path);
			}
			else
				got_it = 0;

			if( !got_it ) {
				if( exists ) // prompt user to overwrite
					confirm_paths.append(cstrdup(new_path));

				needed_assets.append(proxy_asset);
				proxy_asset->add_user();
				needed_orig_assets.append(orig_asset);
				orig_asset->add_user();
//printf("ProxyThread::handle_close_event %d %s\n", __LINE__, new_path);
			}
		}

// test for existing files
		int result = 0;
		if( confirm_paths.size() ) {
			result = ConfirmSave::test_files(mwindow, &confirm_paths);
			confirm_paths.remove_all_objects();
		}

		if( !result ) {
			int canceled = 0;
			failed = 0;

// create proxy assets which don't already exist
			if( needed_orig_assets.size() > 0 ) {
				int64_t total_len = 0;
				for( int i = 0; i < needed_orig_assets.size(); i++ )
					total_len += needed_orig_assets.get(i)->video_length;
// start progress bar.  MWindow is locked inside this
				progress = mwindow->mainprogress->
					start_progress(_("Creating proxy files..."), total_len);
				total_rendered = 0;

				ProxyFarm engine(mwindow, this,
					&needed_assets, &needed_orig_assets);
				engine.process_packages();
printf("failed=%d canceled=%d\n", failed, progress->is_cancelled());

// stop progress bar
				canceled = progress->is_cancelled();
				progress->stop_progress();
				delete progress;  progress = 0;

				if( failed && !canceled ) {
					int cx, cy;
					mwindow->gui->get_abs_cursor_xy(cx, cy, 1);
					ErrorBox error_box(PROGRAM_NAME ": Error", cx, cy);
					error_box.create_objects(_("Error making proxy."));
					error_box.raise_window();
					error_box.run_window();
				}
			}

// resize project
			if( !failed && !canceled ) {
				mwindow->set_proxy(use_scaler, new_scale, &orig_assets, &proxy_assets);
			}
		}

		for( int i = 0; i < proxy_assets.size(); i++ )
			proxy_assets.get(i)->Garbage::remove_user();
		for( int i = 0; i < orig_assets.size(); i++ )
			orig_assets.get(i)->Garbage::remove_user();
		for( int i = 0; i < needed_assets.size(); i++ )
			needed_assets.get(i)->Garbage::remove_user();
		for( int i = 0; i < needed_orig_assets.size(); i++ )
			needed_orig_assets.get(i)->Garbage::remove_user();
	}

	mwindow->undo->update_undo_after(_("proxy"), LOAD_ALL);
	mwindow->edl->Garbage::remove_user();
	mwindow->restart_brender();

	mwindow->gui->lock_window("ProxyThread::to_proxy");
	mwindow->update_project(LOAD_ALL);
	mwindow->gui->unlock_window();
}


void ProxyThread::to_proxy_path(char *new_path, Asset *asset, int scale)
{
	strcpy(new_path, asset->path);
	char prxy[BCTEXTLEN];
	sprintf(prxy, ".proxy%d", scale);
// path is already a proxy
	if( strstr(new_path, prxy) ) return;
	int len = strlen(new_path);
// insert proxy prxy
	char *ptr = strrchr(new_path, '.');
	if( ptr ) {
		char *cp = new_path + len;
		int n = strlen(prxy);
		char *bp = cp + n;
		for( *bp=0; cp>ptr; ) *--bp = *--cp;
		for( cp= prxy+n; bp>ptr; ) *--bp = *--cp;
//printf("ProxyThread::to_proxy_path %d %s %s\n", __LINE__, new_path), asset->path);
	}
	else
		strcpy(new_path+len, prxy);
}

void ProxyThread::from_proxy_path(char *new_path, Asset *asset, int scale)
{
	char prxy[BCTEXTLEN];
	sprintf(prxy, ".proxy%d", scale);
	strcpy(new_path, asset->path);
	char *ptr = strstr(asset->path, prxy);
	if( !ptr ) return;
	int n = strlen(prxy);
	for( char *cp=ptr+n; --n>=0; ++ptr,++cp ) *ptr = *cp;
	*ptr = 0;
}

void ProxyThread::update_progress()
{
	counter_lock->lock();
	total_rendered++;
	counter_lock->unlock();
	progress->update(total_rendered);
}

int ProxyThread::is_canceled()
{
	return progress->is_cancelled();
}



ProxyWindow::ProxyWindow(MWindow *mwindow, ProxyThread *thread, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Proxy settings"), x, y, WIDTH, HEIGHT,
		-1, -1, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	format_tools = 0;
}

ProxyWindow::~ProxyWindow()
{
	lock_window("ProxyWindow::~ProxyWindow");
	delete format_tools;
	unlock_window();
}


void ProxyWindow::create_objects()
{
	lock_window("ProxyWindow::create_objects");

	int margin = mwindow->theme->widget_border;
	int x = margin;
	int y = margin;
	thread->use_scaler = mwindow->edl->session->proxy_use_scaler;
	thread->orig_scale = mwindow->edl->session->proxy_scale;
	thread->new_scale = thread->orig_scale;

	BC_Title *text;
	add_subwindow(text = new BC_Title(x, y, 
		_("What size should the project\n"
		  "be scaled to for editing?")));
	y += text->get_h() * 2 + margin;

	add_subwindow(text = new BC_Title(x, y, _("Scale factor:")));
	x += text->get_w() + margin;

	thread->size_text[0] = cstrdup(_("Original size"));
	thread->size_factors[0] = 1;
	thread->total_sizes = 1;
	int popupmenu_w = BC_PopupMenu::calculate_w(get_text_width(MEDIUMFONT, thread->size_text[0]));
	add_subwindow(scale_factor = new ProxyMenu(mwindow, this, x, y, popupmenu_w, ""));
	scale_factor->update_sizes();
	x += scale_factor->get_w() + margin;

	ProxyTumbler *tumbler;
	add_subwindow(tumbler = new ProxyTumbler(mwindow, this, x, y));
	y += tumbler->get_h() + margin;
	x = margin;
	add_subwindow(use_scaler = new ProxyUseScaler(mwindow, this, x, y));
	y += use_scaler->get_h() + margin;
	y += 25;

	x = margin;
	add_subwindow(text = new BC_Title(x, y, _("New media dimensions: ")));
	x += text->get_w() + margin;
	add_subwindow(new_dimensions = new BC_Title(x, y, ""));

	x = margin;
	y += new_dimensions->get_h() * 2 + margin;


	format_tools = new ProxyFormatTools(mwindow, this, thread->asset);
	format_tools->create_objects(x, y, 0, 1, 0, 0, 0, 1, 0, 1, // skip the path
		0, 0);

	update();

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);
	unlock_window();
}

ProxyFormatTools::ProxyFormatTools(MWindow *mwindow, ProxyWindow *pwindow, Asset *asset)
 : FormatTools(mwindow, pwindow, asset)
{
	this->pwindow = pwindow;
}

void ProxyFormatTools::update_format()
{
        FormatTools::update_format();
	pwindow->use_scaler->update();
}

void ProxyWindow::update()
{
// preview the new size
	char string[BCTEXTLEN];
//printf("ProxyWindow::update %d %d %d %d %d\n", 
// __LINE__, mwindow->edl->session->output_w, mwindow->edl->session->output_h,
// thread->orig_scale, thread->new_scale);
	int orig_w = mwindow->edl->session->output_w * thread->orig_scale;
	int orig_h = mwindow->edl->session->output_h * thread->orig_scale;
	int new_w = orig_w / thread->new_scale;
	if( new_w & 1 ) ++new_w;
	int new_h = orig_h / thread->new_scale;
	if( new_h & 1 ) ++new_h;
	sprintf(string, "%dx%d", new_w, new_h);
	new_dimensions->update(string);
	thread->scale_to_text(string, thread->new_scale);
	scale_factor->set_text(string);
	use_scaler->update();
}


ProxyUseScaler::ProxyUseScaler(MWindow *mwindow, ProxyWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->use_scaler, _("Use scaler   (FFMPEG only)"))
{
	this->mwindow = mwindow;
	this->pwindow = pwindow;
}

void ProxyUseScaler::update()
{
	ProxyThread *thread = pwindow->thread;
	if( thread->asset->format != FILE_FFMPEG ) thread->use_scaler = 0;
	BC_CheckBox::update(thread->use_scaler);
	int scaler_avail = thread->asset->format == FILE_FFMPEG ? 1 : 0;
	if( !scaler_avail &&  enabled ) disable();
	if( scaler_avail  && !enabled ) enable();
}

int ProxyUseScaler::handle_event()
{
	pwindow->thread->new_scale = 1;
	pwindow->thread->use_scaler = get_value();
	pwindow->scale_factor->update_sizes();
	pwindow->update();
	return 1;
}


ProxyMenu::ProxyMenu(MWindow *mwindow, ProxyWindow *pwindow,
		int x, int y, int w, const char *text)
 : BC_PopupMenu(x, y, w, text, 1)
{
	this->mwindow = mwindow;
	this->pwindow = pwindow;
}

void ProxyMenu::update_sizes()
{
	while( total_items() > 0 ) del_item(0);
	ProxyThread *thread = pwindow->thread;
	thread->calculate_sizes();
	for( int i=0; i < thread->total_sizes; i++ )
		add_item(new BC_MenuItem(thread->size_text[i]));
}

int ProxyMenu::handle_event()
{
	for( int i = 0; i < pwindow->thread->total_sizes; i++ ) {
		if( !strcmp(get_text(), pwindow->thread->size_text[i]) ) {
			pwindow->thread->new_scale = pwindow->thread->size_factors[i];
			if( pwindow->thread->new_scale == 1 )
				pwindow->thread->use_scaler = 0;
			pwindow->update();
			break;
		}
	}
	return 1;
}


ProxyTumbler::ProxyTumbler(MWindow *mwindow, ProxyWindow *pwindow, int x, int y)
 : BC_Tumbler(x, y, 0)
{
	this->mwindow = mwindow;
	this->pwindow = pwindow;
}

int ProxyTumbler::handle_up_event()
{
	if( pwindow->thread->new_scale > 1 ) {
		int i;
		for( i = 0; i < pwindow->thread->total_sizes; i++ ) {
			if( pwindow->thread->new_scale == pwindow->thread->size_factors[i] ) {
				i--;
				pwindow->thread->new_scale = pwindow->thread->size_factors[i];
				pwindow->update();
				return 1;
			}
		}		
	}

	return 0;
}

int ProxyTumbler::handle_down_event()
{
	int i;
	for( i = 0; i < pwindow->thread->total_sizes - 1; i++ ) {
		if( pwindow->thread->new_scale == pwindow->thread->size_factors[i] ) {
			i++;
			pwindow->thread->new_scale = pwindow->thread->size_factors[i];
			pwindow->update();
			return 1;
		}
	}

	return 0;
}


ProxyPackage::ProxyPackage()
 : LoadPackage()
{
}

ProxyClient::ProxyClient(MWindow *mwindow, ProxyThread *thread, ProxyFarm *server)
 : LoadClient(server)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

void ProxyClient::process_package(LoadPackage *ptr)
{
	ProxyPackage *package = (ProxyPackage*)ptr;
	if( thread->failed ) return;

	File src_file;
	File dst_file;
	EDL *edl = mwindow->edl;
	Preferences *preferences = mwindow->preferences;
	int processors = 1;

	int result;
	src_file.set_processors(processors);
	src_file.set_preload(edl->session->playback_preload);
	src_file.set_subtitle(edl->session->decode_subtitles ? 
		edl->session->subtitle_number : -1);
	src_file.set_interpolate_raw(edl->session->interpolate_raw);
	src_file.set_white_balance_raw(edl->session->white_balance_raw);

//printf("%s %s\n", package->orig_asset->path, package->proxy_asset->path);


	result = src_file.open_file(preferences, package->orig_asset, 1, 0);
	if( result ) {
// go to the next asset if the reader fails
//		thread->failed = 1;
		return;
	}

	dst_file.set_processors(processors);
	result = dst_file.open_file(preferences, package->proxy_asset, 0, 1);
	if( result ) {
		thread->failed = 1;
		return;
	}

	dst_file.start_video_thread(1, edl->session->color_model,
			processors > 1 ? 2 : 1, 0);

	VFrame src_frame(0, -1,
		package->orig_asset->width, package->orig_asset->height, 
		edl->session->color_model, -1);

	OverlayFrame scaler(processors);

	for( int64_t i = 0; i < package->orig_asset->video_length &&
	     !thread->failed && !thread->is_canceled(); i++ ) {
		src_file.set_video_position(i, 0);
		result = src_file.read_frame(&src_frame);
//printf("result=%d\n", result);

		if( result ) {
// go to the next asset if the reader fails
//			thread->failed = 1;
			break;
		}

// have to write after getting the video buffer or it locks up
		VFrame ***dst_frames = dst_file.get_video_buffer();
		VFrame *dst_frame = dst_frames[0][0];
		scaler.overlay(dst_frame, &src_frame,
			0, 0, src_frame.get_w(), src_frame.get_h(),
			0, 0, dst_frame->get_w(), dst_frame->get_h(),
			1.0, TRANSFER_REPLACE, NEAREST_NEIGHBOR);
		result = dst_file.write_video_buffer(1);
		if( result ) {
// only fail if the writer fails
			thread->failed = 1;
			break;
		}
		else {
			thread->update_progress();
		}
	}
}


ProxyFarm::ProxyFarm(MWindow *mwindow, ProxyThread *thread,
	ArrayList<Asset*> *proxy_assets, ArrayList<Asset*> *orig_assets)
 : LoadServer(MIN(mwindow->preferences->processors, proxy_assets->size()), 
 	proxy_assets->size())
{
	this->mwindow = mwindow;
	this->thread = thread;
	this->proxy_assets = proxy_assets;
	this->orig_assets = orig_assets;
}

void ProxyFarm::init_packages()
{
	for( int i = 0; i < get_total_packages(); i++ ) {
    	ProxyPackage *package = (ProxyPackage*)get_package(i);
    	package->proxy_asset = proxy_assets->get(i);
		package->orig_asset = orig_assets->get(i);
	}
}

LoadClient* ProxyFarm::new_client()
{
	return new ProxyClient(mwindow, thread, this);
}

LoadPackage* ProxyFarm::new_package()
{
	return new ProxyPackage;
}

