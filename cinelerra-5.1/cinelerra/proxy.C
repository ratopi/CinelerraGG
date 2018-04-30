
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
#include "cache.h"
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
#include "mainerror.h"
#include "mainindexes.h"
#include "mainprogress.h"
#include "mainundo.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "overlayframe.h"
#include "preferences.h"
#include "proxy.h"
#include "renderengine.h"
#include "theme.h"
#include "transportque.h"
#include "vrender.h"

#define WIDTH 400
#define HEIGHT 285
#define MAX_SCALE 16

ProxyMenuItem::ProxyMenuItem(MWindow *mwindow)
 : BC_MenuItem(_("Proxy settings..."),  _("Alt-r"), 'r')
{
	this->mwindow = mwindow;
	set_alt();
	dialog = 0;
}
ProxyMenuItem::~ProxyMenuItem()
{
	delete dialog;
}

void ProxyMenuItem::create_objects()
{
	dialog = new ProxyDialog(mwindow);
}

int ProxyMenuItem::handle_event()
{
	mwindow->gui->unlock_window();
	dialog->start();
	mwindow->gui->lock_window("ProxyMenuItem::handle_event");

	return 1;
}

ProxyDialog::ProxyDialog(MWindow *mwindow)
{
	this->mwindow = mwindow;
	gui = 0;
	asset = new Asset;
	bzero(size_text, sizeof(char*) * MAX_SIZES);
	bzero(size_factors, sizeof(int) * MAX_SIZES);
	size_text[0] = cstrdup(_("Original size"));
	size_factors[0] = 1;
	total_sizes = 1;
}

ProxyDialog::~ProxyDialog()
{
	close_window();
	for( int i=0; i<MAX_SIZES; ++i ) delete [] size_text[i];
	asset->remove_user();
}

BC_Window* ProxyDialog::new_gui()
{
	asset->format = FILE_FFMPEG;
	asset->load_defaults(mwindow->defaults, "PROXY_", 1, 1, 0, 0, 0);
	mwindow->gui->lock_window("ProxyDialog::new_gui");
	int cx, cy;
	mwindow->gui->get_abs_cursor(cx, cy);
	gui = new ProxyWindow(mwindow, this, cx - WIDTH/2, cy - HEIGHT/2);
	gui->create_objects();
	mwindow->gui->unlock_window();
	return gui;
}

void ProxyDialog::scale_to_text(char *string, int scale)
{
	strcpy(string, size_text[0]);
	for( int i = 0; i < total_sizes; i++ ) {
		if( scale == size_factors[i] ) {
			strcpy(string, size_text[i]);
			break;
		}
	}
}


void ProxyDialog::calculate_sizes()
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

void ProxyDialog::handle_close_event(int result)
{
	asset->save_defaults(mwindow->defaults, "PROXY_", 1, 1, 0, 0, 0); 

	if( !result )
		to_proxy();
}

void ProxyDialog::to_proxy()
{
	ArrayList<Indexable*> orig_idxbls;
	ArrayList<Indexable*> proxy_assets;

	EDL *edl = mwindow->edl;
	mwindow->edl->Garbage::add_user();
	mwindow->save_backup();
	mwindow->undo->update_undo_before(_("proxy"), this);
	ProxyRender proxy_render(mwindow, asset);

// revert project to original size from current size
// remove all session proxy assets at the at the current proxy_scale
	int proxy_scale = edl->session->proxy_scale;
	if( proxy_scale > 1 ) {
		Asset *orig_asset = edl->assets->first;
		for( ; orig_asset; orig_asset=orig_asset->next ) {
			char new_path[BCTEXTLEN];
			proxy_render.to_proxy_path(new_path, orig_asset, proxy_scale);
// test if proxy asset was already added to proxy_assets
			int got_it = 0;
			for( int i = 0; !got_it && i<proxy_assets.size(); ++i )
				got_it = !strcmp(proxy_assets[i]->path, new_path);
			if( got_it ) continue;
			Asset *proxy_asset = edl->assets->get_asset(new_path);
			if( !proxy_asset ) continue;
// add pointer to existing EDL asset if it exists
// EDL won't delete it unless it's the same pointer.
			proxy_assets.append(proxy_asset);
			proxy_asset->add_user();
			orig_idxbls.append(orig_asset);
			orig_asset->add_user();
		}

// convert from the proxy assets to the original assets
		int proxy_auto_scale = edl->session->proxy_auto_scale;
		mwindow->set_proxy(0, 1, proxy_auto_scale, &proxy_assets, &orig_idxbls);

// remove the references
		for( int i=0; i<proxy_assets.size(); ++i ) {
			Asset *proxy = (Asset *) proxy_assets[i];
			proxy->width = proxy->actual_width;
			proxy->height = proxy->actual_height;
			proxy->remove_user();
			mwindow->edl->assets->remove_pointer(proxy);
			proxy->remove_user();
		}
		proxy_assets.remove_all();
		for( int i = 0; i < orig_idxbls.size(); i++ )
			orig_idxbls[i]->remove_user();
		orig_idxbls.remove_all();
	}

	ArrayList<char *> confirm_paths;    // test for new files
	confirm_paths.set_array_delete();

// convert to new size if not original size
	if( new_scale != 1 ) {
		FileSystem fs;
		Asset *orig = mwindow->edl->assets->first;
		for( ; orig; orig=orig->next ) {
			Asset *proxy = proxy_render.add_original(orig, new_scale);
			if( !proxy ) continue;
			int exists = fs.get_size(proxy->path) > 0 ? 1 : 0;
			int got_it = exists && // if proxy exists, and is newer than orig
			    fs.get_date(proxy->path) > fs.get_date(orig->path) ? 1 : 0;
			if( !got_it ) {
				if( exists ) // prompt user to overwrite
					confirm_paths.append(cstrdup(proxy->path));
				proxy_render.add_needed(orig, proxy);
			}
		}
	}

	int result = 0;
// test for existing files
	if( confirm_paths.size() ) {
		result = ConfirmSave::test_files(mwindow, &confirm_paths);
		confirm_paths.remove_all_objects();
	}

	if( !result )
		result = proxy_render.create_needed_proxies(new_scale);

	if( !result ) // resize project
		mwindow->set_proxy(use_scaler, new_scale, auto_scale,
			&proxy_render.orig_idxbls, &proxy_render.orig_proxies);

	mwindow->undo->update_undo_after(_("proxy"), LOAD_ALL);
	mwindow->edl->Garbage::remove_user();
	mwindow->restart_brender();

	mwindow->gui->lock_window("ProxyDialog::to_proxy");
	mwindow->update_project(LOADMODE_REPLACE);
	mwindow->gui->unlock_window();
}


void ProxyRender::to_proxy_path(char *new_path, Indexable *indexable, int scale)
{
// path is already a proxy
	if( strstr(indexable->path, ".proxy") ) return;
	strcpy(new_path, indexable->path);
	char prxy[BCSTRLEN];
	int n = sprintf(prxy, ".proxy%d", scale);
// insert proxy, path.sfx => path.proxy#-sfx.ext
	char *ep = new_path + strlen(new_path);
	char *sfx = strrchr(new_path, '.');
	if( sfx ) {
		char *bp = ep, *cp = (ep += n);
		while( --bp > sfx ) *--cp = *bp;
		*--cp = '-';
	}
	else {
		sfx = ep;  ep += n;
	}
	for( char *cp=prxy; --n>=0; ++cp ) *sfx++ = *cp;
	const char *ext = format_asset->format == FILE_FFMPEG ?
		format_asset->fformat : File::get_tag(format_asset->format);
	*ep++ = '.';
	while( *ext ) *ep++ = *ext++;
	*ep = 0;
//printf("ProxyRender::to_proxy_path %d %s %s\n", __LINE__, new_path), asset->path);
}

void ProxyRender::from_proxy_path(char *new_path, Asset *asset, int scale)
{
	char prxy[BCTEXTLEN];
	int n = sprintf(prxy, ".proxy%d", scale);
	strcpy(new_path, asset->path);
	char *ptr = strstr(new_path, prxy);
	if( !ptr || (ptr[n] != '-' && ptr[n] != '.') ) return;
// remove proxy, path.proxy#-sfx.ext => path.sfx
	char *ext = strrchr(ptr, '.');
	if( !ext ) ext = ptr + strlen(ptr);
	char *cp = ptr + n;
	for( *cp='.'; cp<ext; ++cp ) *ptr++ = *cp;
	*ptr = 0;
}

ProxyRender::ProxyRender(MWindow *mwindow, Asset *format_asset)
{
	this->mwindow = mwindow;
	this->format_asset = format_asset;
	progress = 0;
	counter_lock = new Mutex("ProxyDialog::counter_lock");
	total_rendered = 0;
	failed = 0;  canceled = 0;
}

ProxyRender::~ProxyRender()
{
	delete progress;
	delete counter_lock;

	for( int i=0,n=orig_idxbls.size(); i<n; ++i ) orig_idxbls[i]->remove_user();
	for( int i=0,n=orig_proxies.size(); i<n; ++i ) orig_proxies[i]->remove_user();
	for( int i=0,n=needed_idxbls.size(); i<n; ++i ) needed_idxbls[i]->remove_user();
	for( int i=0,n=needed_proxies.size(); i<n; ++i ) needed_proxies[i]->remove_user();
}

Asset *ProxyRender::add_original(Indexable *idxbl, int new_scale)
{
	if( !idxbl->have_video() ) return 0;
	if( idxbl->get_video_frames() <= 0 ) return 0;
// don't proxy proxies
	if( strstr(idxbl->path,".proxy") ) return 0;
	char new_path[BCTEXTLEN];
	to_proxy_path(new_path, idxbl, new_scale);
// don't proxy if not readable
	if( idxbl->is_asset && access(idxbl->path, R_OK) ) return 0;
// add to orig_idxbls & orig_proxies if it isn't already there.
	int got_it = 0;
	for( int i = 0; !got_it && i<orig_proxies.size(); ++i )
		got_it = !strcmp(orig_proxies[i]->path, new_path);
	if( got_it ) return 0;
	Assets *edl_assets = mwindow->edl->assets;
	Asset *proxy = edl_assets->get_asset(new_path);
	if( !proxy ) {
		proxy = new Asset(new_path);
// new compression parameters
		proxy->copy_format(format_asset, 0);
		proxy->awindow_folder = AW_PROXY_FOLDER;
		proxy->audio_data = 0;
		proxy->video_data = 1;
		proxy->layers = 1;
		proxy->width = idxbl->get_w() / new_scale;
		if( proxy->width & 1 ) ++proxy->width;
		proxy->actual_width = proxy->width;
		proxy->height = idxbl->get_h() / new_scale;
		if( proxy->height & 1 ) ++proxy->height;
		proxy->actual_height = proxy->height;
		proxy->frame_rate = idxbl->get_frame_rate();
		proxy->video_length = idxbl->get_video_frames();
		edl_assets->append(proxy);
	}
	proxy->add_user();
	orig_proxies.append(proxy);
	idxbl->add_user();
	orig_idxbls.append(idxbl);
	return proxy;
}

void ProxyRender::add_needed(Indexable *idxbl, Asset *proxy)
{
	needed_idxbls.append(idxbl);
	idxbl->add_user();
	needed_proxies.append(proxy);
	proxy->add_user();
}

void ProxyRender::update_progress()
{
	counter_lock->lock();
	++total_rendered;
	counter_lock->unlock();
	progress->update(total_rendered);
}

int ProxyRender::is_canceled()
{
	return progress->is_cancelled();
}

int ProxyRender::create_needed_proxies(int new_scale)
{
	if( !needed_proxies.size() ) return 0;
	total_rendered = 0;
	failed = 0;  canceled = 0;

// create proxy assets which don't already exist
	int64_t total_len = 0;
	for( int i = 0; i < needed_idxbls.size(); i++ ) {
		total_len += needed_idxbls[i]->get_video_frames();
	}

// start progress bar.  MWindow is locked inside this
	progress = mwindow->mainprogress->
		start_progress(_("Creating proxy files..."), total_len);

	ProxyFarm engine(mwindow, this, &needed_idxbls, &needed_proxies);
	engine.process_packages();
printf("failed=%d canceled=%d\n", failed, progress->is_cancelled());

// stop progress bar
	canceled = progress->is_cancelled();
	progress->stop_progress();
	delete progress;  progress = 0;

	if( failed && !canceled ) {
		eprintf("Error making proxy.");
	}

	return !failed && !canceled ? 0 : 1;
}


ProxyWindow::ProxyWindow(MWindow *mwindow, ProxyDialog *dialog, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Proxy settings"), x, y, WIDTH, HEIGHT,
		-1, -1, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->dialog = dialog;
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

	dialog->use_scaler = mwindow->edl->session->proxy_use_scaler;
	dialog->orig_scale = mwindow->edl->session->proxy_scale;
	dialog->auto_scale = mwindow->edl->session->proxy_auto_scale;
	dialog->new_scale = dialog->orig_scale;

	int x = margin;
	int y = margin+10;
	add_subwindow(use_scaler = new ProxyUseScaler(this, x, y));
	y += use_scaler->get_h() + margin;

	BC_Title *text;
	add_subwindow(text = new BC_Title(x, y, _("Scale factor:")));
	x += text->get_w() + margin;

	int popupmenu_w = BC_PopupMenu::calculate_w(get_text_width(MEDIUMFONT, dialog->size_text[0]));
	add_subwindow(scale_factor = new ProxyMenu(mwindow, this, x, y, popupmenu_w, ""));
	scale_factor->update_sizes();
	x += scale_factor->get_w() + margin;

	ProxyTumbler *tumbler;
	add_subwindow(tumbler = new ProxyTumbler(mwindow, this, x, y));
	y += tumbler->get_h() + margin;

	x = margin;
	add_subwindow(text = new BC_Title(x, y, _("New media dimensions: ")));
	x += text->get_w() + margin;
	add_subwindow(new_dimensions = new BC_Title(x, y, ""));
	y += new_dimensions->get_h() + margin;

	x = margin;  y += 25;
	format_tools = new ProxyFormatTools(mwindow, this, dialog->asset);
	format_tools->create_objects(x, y, 0, 1, 0, 0, 0, 1, 0, 1, // skip the path
		0, 0);

	x = margin;
	add_subwindow(auto_scale = new ProxyAutoScale(this, x, y));
	y += auto_scale->get_h() + margin;

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
// dialog->orig_scale, dialog->new_scale);
	int orig_w = mwindow->edl->session->output_w * dialog->orig_scale;
	int orig_h = mwindow->edl->session->output_h * dialog->orig_scale;
	int new_w = orig_w / dialog->new_scale;
	if( new_w & 1 ) ++new_w;
	int new_h = orig_h / dialog->new_scale;
	if( new_h & 1 ) ++new_h;
	sprintf(string, "%dx%d", new_w, new_h);
	new_dimensions->update(string);
	dialog->scale_to_text(string, dialog->new_scale);
	scale_factor->set_text(string);
	use_scaler->update();
	auto_scale->update();
}


ProxyUseScaler::ProxyUseScaler(ProxyWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->dialog->use_scaler, _("Use scaler   (FFMPEG only)"))
{
	this->pwindow = pwindow;
}

void ProxyUseScaler::update()
{
	ProxyDialog *dialog = pwindow->dialog;
	if( dialog->asset->format != FILE_FFMPEG ) dialog->use_scaler = 0;
	BC_CheckBox::update(dialog->use_scaler);
	int scaler_avail = dialog->asset->format == FILE_FFMPEG ? 1 : 0;
	if( !scaler_avail &&  enabled ) disable();
	if( scaler_avail  && !enabled ) enable();
}

int ProxyUseScaler::handle_event()
{
	pwindow->dialog->new_scale = 1;
	pwindow->dialog->use_scaler = get_value();
	pwindow->scale_factor->update_sizes();
	pwindow->update();
	return 1;
}

ProxyAutoScale::ProxyAutoScale(ProxyWindow *pwindow, int x, int y)
 : BC_CheckBox(x, y, pwindow->dialog->use_scaler, _("Auto proxy/scale media loads"))
{
	this->pwindow = pwindow;
}

void ProxyAutoScale::update()
{
	ProxyDialog *dialog = pwindow->dialog;
	if( dialog->new_scale == 1 ) dialog->auto_scale = 0;
	BC_CheckBox::update(dialog->auto_scale);
	int can_auto_proxy = dialog->new_scale != 1 ? 1 : 0;
	if( !can_auto_proxy &&  enabled ) disable();
	if( can_auto_proxy  && !enabled ) enable();
}

int ProxyAutoScale::handle_event()
{
	pwindow->dialog->auto_scale = get_value();
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
	ProxyDialog *dialog = pwindow->dialog;
	dialog->calculate_sizes();
	for( int i=0; i < dialog->total_sizes; i++ )
		add_item(new BC_MenuItem(dialog->size_text[i]));
}

int ProxyMenu::handle_event()
{
	ProxyDialog *dialog = pwindow->dialog;
	for( int i = 0; i < dialog->total_sizes; i++ ) {
		if( !strcmp(get_text(), pwindow->dialog->size_text[i]) ) {
			dialog->new_scale = pwindow->dialog->size_factors[i];
			if( dialog->new_scale == 1 ) dialog->use_scaler = 0;
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
	if( pwindow->dialog->new_scale > 1 ) {
		int i;
		for( i = 0; i < pwindow->dialog->total_sizes; i++ ) {
			if( pwindow->dialog->new_scale == pwindow->dialog->size_factors[i] ) {
				i--;
				pwindow->dialog->new_scale = pwindow->dialog->size_factors[i];
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
	for( i = 0; i < pwindow->dialog->total_sizes - 1; i++ ) {
		if( pwindow->dialog->new_scale == pwindow->dialog->size_factors[i] ) {
			i++;
			pwindow->dialog->new_scale = pwindow->dialog->size_factors[i];
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

ProxyClient::ProxyClient(MWindow *mwindow,
		ProxyRender *proxy_render, ProxyFarm *server)
 : LoadClient(server)
{
	this->mwindow = mwindow;
	this->proxy_render = proxy_render;
	render_engine = 0;
	video_cache = 0;
	src_file = 0;
}
ProxyClient::~ProxyClient()
{
	delete render_engine;
	delete video_cache;
	delete src_file;
}

void ProxyClient::process_package(LoadPackage *ptr)
{
	if( proxy_render->failed ) return;
	if( proxy_render->is_canceled() ) return;

	EDL *edl = mwindow->edl;
	Preferences *preferences = mwindow->preferences;
	ProxyPackage *package = (ProxyPackage*)ptr;
	Indexable *orig = package->orig_idxbl;
	Asset *proxy = package->proxy_asset;
//printf("%s %s\n", orig->path, proxy->path);
	VRender *vrender = 0;
	int jobs = proxy_render->needed_proxies.size();
	int processors = preferences->project_smp / jobs + 1, result = 0;

	if( orig->is_asset ) {
		src_file = new File;
		src_file->set_processors(processors);
		src_file->set_preload(edl->session->playback_preload);
		src_file->set_subtitle(edl->session->decode_subtitles ? 
			edl->session->subtitle_number : -1);
		src_file->set_interpolate_raw(edl->session->interpolate_raw);
		src_file->set_white_balance_raw(edl->session->white_balance_raw);
		if( src_file->open_file(preferences, (Asset*)orig, 1, 0) != FILE_OK )
			result = 1;
	}
	else {
		TransportCommand command;
		command.command = CURRENT_FRAME;
		command.get_edl()->copy_all((EDL *)orig);
		command.change_type = CHANGE_ALL;
		command.realtime = 0;
		render_engine = new RenderEngine(0, preferences, 0, 0);
	        render_engine->set_vcache(video_cache = new CICache(preferences));
		render_engine->arm_command(&command);
		if( !(vrender = render_engine->vrender) )
			result = 1;
	}
	if( result ) {
// go to the next asset if the reader fails
//		proxy_render->failed = 1;
		return;
	}

	File dst_file;
	dst_file.set_processors(processors);
	result = dst_file.open_file(preferences, proxy, 0, 1);
	if( result ) {
		proxy_render->failed = 1;
		return;
	}

	dst_file.start_video_thread(1, edl->session->color_model,
			processors > 1 ? 2 : 1, 0);

	int src_w = orig->get_w(), src_h = orig->get_h();
	VFrame src_frame(src_w,src_h, edl->session->color_model);

	OverlayFrame scaler(processors);

	for( int64_t i=0, length=orig->get_video_frames(); i<length &&
	     !proxy_render->failed && !proxy_render->is_canceled(); ++i ) {
		if( orig->is_asset ) {
			src_file->set_video_position(i, 0);
			result = src_file->read_frame(&src_frame);
		}
		else
			result = vrender->process_buffer(&src_frame, i, 0);
//printf("result=%d\n", result);

		if( result ) {
// go to the next asset if the reader fails
//			proxy_render->failed = 1;
			break;
		}

// have to write after getting the video buffer or it locks up
		VFrame ***dst_frames = dst_file.get_video_buffer();
		VFrame *dst_frame = dst_frames[0][0];
		int dst_w = dst_frame->get_w(), dst_h = dst_frame->get_h();
		scaler.overlay(dst_frame, &src_frame,
			0,0, src_w,src_h, 0,0, dst_w,dst_h,
			1.0, TRANSFER_REPLACE, NEAREST_NEIGHBOR);
		result = dst_file.write_video_buffer(1);
		if( result ) {
// only fail if the writer fails
			proxy_render->failed = 1;
			break;
		}
		proxy_render->update_progress();
	}
	if( !proxy_render->failed && !proxy_render->is_canceled() ) {
		Asset *asset = mwindow->edl->assets->update(proxy);
		mwindow->mainindexes->add_next_asset(0, asset);
		mwindow->mainindexes->start_build();
	}
}


ProxyFarm::ProxyFarm(MWindow *mwindow, ProxyRender *proxy_render,
		ArrayList<Indexable*> *orig_idxbls,
		ArrayList<Asset*> *proxy_assets)
 : LoadServer(MIN(mwindow->preferences->processors, proxy_assets->size()), 
 	proxy_assets->size())
{
	this->mwindow = mwindow;
	this->proxy_render = proxy_render;
	this->orig_idxbls = orig_idxbls;
	this->proxy_assets = proxy_assets;
}

void ProxyFarm::init_packages()
{
	for( int i = 0; i < get_total_packages(); i++ ) {
	    	ProxyPackage *package = (ProxyPackage*)get_package(i);
    		package->proxy_asset = proxy_assets->get(i);
		package->orig_idxbl = orig_idxbls->get(i);
	}
}

LoadClient* ProxyFarm::new_client()
{
	return new ProxyClient(mwindow, proxy_render, this);
}

LoadPackage* ProxyFarm::new_package()
{
	return new ProxyPackage;
}

