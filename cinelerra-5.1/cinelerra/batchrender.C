
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "batchrender.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "confirmsave.h"
#include "cstrdup.h"
#include "bchash.h"
#include "edits.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "keyframe.h"
#include "keys.h"
#include "labels.h"
#include "language.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "packagedispatcher.h"
#include "packagerenderer.h"
#include "plugin.h"
#include "pluginset.h"
#include "preferences.h"
#include "render.h"
#include "theme.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"


static const char *list_titles[] =
{
	_("Enabled"),
	_("Output"),
	_("EDL"),
	_("Elapsed")
};

static int list_widths[] =
{
	50,
	100,
	200,
	100
};

BatchRenderMenuItem::BatchRenderMenuItem(MWindow *mwindow)
 : BC_MenuItem(_("Batch Render..."), _("Shift-B"), 'B')
{
	set_shift(1);
	this->mwindow = mwindow;
}

int BatchRenderMenuItem::handle_event()
{
	mwindow->batch_render->start();
	return 1;
}








BatchRenderJob::BatchRenderJob(Preferences *preferences)
{
	this->preferences = preferences;
	asset = new Asset;
	edl_path[0] = 0;
	strategy = 0;
	enabled = 1;
	elapsed = 0;
}

BatchRenderJob::~BatchRenderJob()
{
	asset->Garbage::remove_user();
}

void BatchRenderJob::copy_from(BatchRenderJob *src)
{
	asset->copy_from(src->asset, 0);
	strcpy(edl_path, src->edl_path);
	strategy = src->strategy;
	enabled = src->enabled;
	elapsed = 0;
}

void BatchRenderJob::load(FileXML *file)
{
	int result = 0;

	edl_path[0] = 0;
	file->tag.get_property("EDL_PATH", edl_path);
	strategy = file->tag.get_property("STRATEGY", strategy);
	enabled = file->tag.get_property("ENABLED", enabled);
	elapsed = file->tag.get_property("ELAPSED", elapsed);
	fix_strategy();

	result = file->read_tag();
	if(!result)
	{
		if(file->tag.title_is("ASSET"))
		{
			file->tag.get_property("SRC", asset->path);
			asset->read(file, 0);
// The compression parameters are stored in the defaults to reduce
// coding maintenance.  The defaults must now be stuffed into the XML for
// unique storage.
			BC_Hash defaults;
			defaults.load_string(file->read_text());
			asset->load_defaults(&defaults,
				"",
				0,
				1,
				0,
				0,
				0);
		}
	}
}

void BatchRenderJob::save(FileXML *file)
{
	file->tag.set_property("EDL_PATH", edl_path);
	file->tag.set_property("STRATEGY", strategy);
	file->tag.set_property("ENABLED", enabled);
	file->tag.set_property("ELAPSED", elapsed);
	file->append_tag();
	file->append_newline();
	asset->write(file,
		0,
		"");

// The compression parameters are stored in the defaults to reduce
// coding maintenance.  The defaults must now be stuffed into the XML for
// unique storage.
	BC_Hash defaults;
	asset->save_defaults(&defaults,
		"",
		0,
		1,
		0,
		0,
		0);
	char *string;
	defaults.save_string(string);
	file->append_text(string);
	free(string);
	file->tag.set_title("/JOB");
	file->append_tag();
	file->append_newline();
}

void BatchRenderJob::fix_strategy()
{
	strategy = Render::fix_strategy(strategy, preferences->use_renderfarm);
}










BatchRenderThread::BatchRenderThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	current_job = 0;
	rendering_job = -1;
	is_rendering = 0;
	default_job = 0;
	boot_defaults = 0;
	preferences = 0;
	warn = 1;
	render = 0;
	file_entries = 0;
}

BatchRenderThread::BatchRenderThread()
 : BC_DialogThread()
{
	mwindow = 0;
	current_job = 0;
	rendering_job = -1;
	is_rendering = 0;
	default_job = 0;
	boot_defaults = 0;
	preferences = 0;
	warn = 1;
	render = 0;
	file_entries = 0;
}

BatchRenderThread::~BatchRenderThread()
{
	close_window();
	delete boot_defaults;
	delete preferences;
	delete render;
	if( file_entries ) {
		file_entries->remove_all_objects();
		delete file_entries;
	}
}

void BatchRenderThread::reset(int warn)
{
	if( warn ) this->warn = 1;
	current_job = 0;
	rendering_job = -1;
	delete default_job;  default_job = 0;
	jobs.remove_all_objects();
	if(file_entries) {
		file_entries->remove_all_objects();
		delete file_entries;  file_entries = 0;
	}
}

void BatchRenderThread::handle_close_event(int result)
{
// Save settings
	char path[BCTEXTLEN];
	path[0] = 0;
	save_jobs(path);
	save_defaults(mwindow->defaults);
	reset();
}

BC_Window* BatchRenderThread::new_gui()
{
	current_start = 0.0;
	current_end = 0.0;
	default_job = new BatchRenderJob(mwindow->preferences);


	if(!file_entries)
	{
		file_entries = new ArrayList<BC_ListBoxItem*>;
		FileSystem fs;
		char string[BCTEXTLEN];
	// Load current directory
		fs.update(getcwd(string, BCTEXTLEN));
		for(int i = 0; i < fs.total_files(); i++)
		{
			file_entries->append(
				new BC_ListBoxItem(
					fs.get_entry(i)->get_name()));
		}
	}

	char path[BCTEXTLEN];
	path[0] = 0;
	load_jobs(path, mwindow->preferences);
	load_defaults(mwindow->defaults);
	this->gui = new BatchRenderGUI(mwindow,
		this,
		mwindow->session->batchrender_x,
		mwindow->session->batchrender_y,
		mwindow->session->batchrender_w,
		mwindow->session->batchrender_h);
	this->gui->create_objects();
	return this->gui;
}


void BatchRenderThread::load_jobs(char *path, Preferences *preferences)
{
	FileXML file;
	int result = 0;

	jobs.remove_all_objects();
	if(path[0])
		file.read_from_file(path);
	else
		file.read_from_file(create_path(path));

	while(!result)
	{
		if(!(result = file.read_tag()))
		{
			if(file.tag.title_is("JOBS"))
			{
				warn = file.tag.get_property("WARN", 1);
			}
			else if(file.tag.title_is("JOB"))
			{
				BatchRenderJob *job;
				jobs.append(job = new BatchRenderJob(preferences));
				job->load(&file);
			}
		}
	}
}

void BatchRenderThread::save_jobs(char *path)
{
	FileXML file;
	file.tag.set_title("JOBS");
	file.tag.set_property("WARN", warn);
	file.append_tag();
	file.append_newline();

	for(int i = 0; i < jobs.total; i++)
	{
		file.tag.set_title("JOB");
		jobs.values[i]->save(&file);
	}
	file.tag.set_title("/JOBS");
	file.append_tag();
	file.append_newline();

	if(path[0])
		file.write_to_file(path);
	else
		file.write_to_file(create_path(path));
}

void BatchRenderThread::load_defaults(BC_Hash *defaults)
{
	if(default_job)
	{
		default_job->asset->load_defaults(defaults,
			"BATCHRENDER_", 1, 1, 1, 1, 1);
		default_job->fix_strategy();
	}

	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "BATCHRENDER_COLUMN%d", i);
		column_width[i] = defaults->get(string, list_widths[i]);
	}
}

void BatchRenderThread::save_defaults(BC_Hash *defaults)
{
	if(default_job)
	{
		default_job->asset->save_defaults(defaults,
			"BATCHRENDER_", 1, 1, 1, 1, 1);
		defaults->update("BATCHRENDER_STRATEGY", default_job->strategy);
	}
	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		char string[BCTEXTLEN];
		sprintf(string, "BATCHRENDER_COLUMN%d", i);
		defaults->update(string, column_width[i]);
	}
//	defaults->update("BATCHRENDER_JOB", current_job);
	if(mwindow)
		mwindow->save_defaults();
	else
		defaults->save();
}

char* BatchRenderThread::create_path(char *string)
{
	FileSystem fs;
	sprintf(string, "%s/", File::get_config_path());
	fs.complete_path(string);
	strcat(string, BATCH_PATH);
	return string;
}

void BatchRenderThread::new_job()
{
	BatchRenderJob *result = new BatchRenderJob(mwindow->preferences);
	result->copy_from(get_current_job());
	jobs.append(result);
	current_job = jobs.total - 1;
	gui->create_list(1);
	gui->change_job();
}

void BatchRenderThread::delete_job()
{
	if(current_job < jobs.total && current_job >= 0)
	{
		jobs.remove_object_number(current_job);
		if(current_job > 0) current_job--;
		gui->create_list(1);
		gui->change_job();
	}
}

void BatchRenderThread::use_current_edl()
{
// printf("BatchRenderThread::use_current_edl %d %p %s\n",
// __LINE__,
// mwindow->edl->path,
// mwindow->edl->path);

	strcpy(get_current_edl(), mwindow->edl->path);
	gui->create_list(1);
	gui->edl_path_text->update(get_current_edl());
}

void BatchRenderThread::update_selected_edl()
{
        FileXML xml_file;
	char *path = get_current_edl();
	EDL *edl = mwindow->edl;
        edl->save_xml(&xml_file, path, 0, 0);
        xml_file.terminate_string();
        if( xml_file.write_to_file(path) ) {
		char msg[BCTEXTLEN];
		sprintf(msg, _("Unable to save: %s"), path);
		MainError::show_error(msg);
	}
}

BatchRenderJob* BatchRenderThread::get_current_job()
{
	BatchRenderJob *result;
	if(current_job >= jobs.total || current_job < 0)
	{
		result = default_job;
	}
	else
	{
		result = jobs.values[current_job];
	}
	return result;
}


Asset* BatchRenderThread::get_current_asset()
{
	return get_current_job()->asset;
}

char* BatchRenderThread::get_current_edl()
{
	return get_current_job()->edl_path;
}


// Test EDL files for existence
int BatchRenderThread::test_edl_files()
{
	int not_equiv = 0, ret = 0;
	const char *path = 0;

	for( int i=0; !ret && i<jobs.size(); ++i ) {
		if( !jobs.values[i]->enabled ) continue;
		const char *path = jobs.values[i]->edl_path;
		int is_script = *path == '@' ? 1 : 0;
		if( is_script ) ++path;
		FILE *fp = fopen(path, "r");
		if( fp ) {
			if( warn && mwindow && !is_script ) {
				fseek(fp, 0, SEEK_END);
				int64_t sz = ftell(fp);
				fseek(fp, 0, SEEK_SET);
				char *bfr = new char[sz+1];
				int64_t len = fread(bfr, 1, sz+1, fp);
				if( len == sz ) {
					FileXML file;  file.set_shared_input(bfr, len);
					EDL *edl = new EDL; edl->create_objects();
					edl->load_xml(&file, LOAD_ALL);
					double pos = edl->equivalent_output(mwindow->edl);
					if( pos >= 0 ) ++not_equiv;
					edl->remove_user();
				}
				else
					ret = 1;
				delete [] bfr;
			}
			fclose(fp);
		}
		else
			ret = 1;
	}

	if( ret ) {
		char string[BCTEXTLEN];
		sprintf(string, _("EDL %s not found.\n"), path);
		if( mwindow ) {
			ErrorBox error_box(_(PROGRAM_NAME ": Error"),
				mwindow->gui->get_abs_cursor_x(1),
				mwindow->gui->get_abs_cursor_y(1));
			error_box.create_objects(string);
			error_box.run_window();
			gui->button_enable();
		}
		else {
			fprintf(stderr, "%s", string);
		}
		is_rendering = 0;
	}
	else if( warn && mwindow && not_equiv > 0 ) {
		fprintf(stderr, _("%d job EDLs do not match session edl\n"), not_equiv);
		char string[BCTEXTLEN], *sp = string;
		sp += sprintf(sp, _("%d job EDLs do not match session edl\n"),not_equiv);
		sp += sprintf(sp, _("press cancel to abandon batch render"));
		mwindow->show_warning(&warn, string);
		if( mwindow->wait_warning() ) {
			gui->button_enable();
			is_rendering = 0;
			ret = 1;
		}
		gui->warning->update(warn);
	}

	return ret;
}

void BatchRenderThread::calculate_dest_paths(ArrayList<char*> *paths,
	Preferences *preferences)
{
	for(int i = 0; i < jobs.total; i++)
	{
		BatchRenderJob *job = jobs.values[i];
		if(job->enabled && *job->edl_path != '@')
		{
			PackageDispatcher *packages = new PackageDispatcher;

// Load EDL
			TransportCommand *command = new TransportCommand;
			FileXML *file = new FileXML;
			file->read_from_file(job->edl_path);

// Use command to calculate range.
			command->command = NORMAL_FWD;
			command->get_edl()->load_xml(file,
				LOAD_ALL);
			command->change_type = CHANGE_ALL;
			command->set_playback_range();
			command->playback_range_adjust_inout();

// Create test packages
			packages->create_packages(mwindow,
				command->get_edl(),
				preferences,
				job->strategy,
				job->asset,
				command->start_position,
				command->end_position,
				0);

// Append output paths allocated to total
			packages->get_package_paths(paths);

// Delete package harness
			delete packages;
			delete command;
			delete file;
		}
	}
}


void BatchRenderThread::start_rendering(char *config_path,
	char *batch_path)
{
	BC_Hash *boot_defaults;
	Preferences *preferences;
	Render *render;
	BC_Signals *signals = new BC_Signals;
	// XXX the above stuff is leaked,
//PRINT_TRACE
// Initialize stuff which MWindow does.
	signals->initialize("/tmp/cinelerra_batch%d.dmp");
	MWindow::init_defaults(boot_defaults, config_path);
	load_defaults(boot_defaults);
	preferences = new Preferences;
	preferences->load_defaults(boot_defaults);
	BC_Signals::set_trap_hook(trap_hook, this);
	BC_Signals::set_catch_segv(preferences->trap_sigsegv);
	BC_Signals::set_catch_intr(0);
	if( preferences->trap_sigsegv ) {
		BC_Trace::enable_locks();
	}
	else {
		BC_Trace::disable_locks();
	}

	MWindow::init_plugins(0, preferences);
	char font_path[BCTEXTLEN];
	strcpy(font_path, preferences->plugin_dir);
	strcat(font_path, "/" FONT_SEARCHPATH);
	BC_Resources::init_fontconfig(font_path);
	BC_WindowBase::get_resources()->vframe_shm = 1;

//PRINT_TRACE
	load_jobs(batch_path, preferences);
	save_jobs(batch_path);
	save_defaults(boot_defaults);

//PRINT_TRACE
// Test EDL files for existence
	if(test_edl_files()) return;

//PRINT_TRACE

// Predict all destination paths
	ArrayList<char*> paths;
	paths.set_array_delete();
	calculate_dest_paths(&paths, preferences);

//PRINT_TRACE
	int result = ConfirmSave::test_files(0, &paths);
	paths.remove_all_objects();
// Abort on any existing file because it's so hard to set this up.
	if(result) return;

//PRINT_TRACE
	render = new Render(0);
//PRINT_TRACE
	render->start_batches(&jobs,
		boot_defaults,
		preferences);
//PRINT_TRACE
}

void BatchRenderThread::start_rendering()
{
	if(is_rendering) return;

	is_rendering = 1;
	char path[BCTEXTLEN];
	path[0] = 0;
	save_jobs(path);
	save_defaults(mwindow->defaults);
	gui->button_disable();

// Test EDL files for existence
	if(test_edl_files()) return;

// Predict all destination paths
	ArrayList<char*> paths;
	calculate_dest_paths(&paths,
		mwindow->preferences);

// Test destination files for overwrite
	int result = ConfirmSave::test_files(mwindow, &paths);
	paths.remove_all_objects();

// User cancelled
	if(result)
	{
		is_rendering = 0;
		gui->button_enable();
		return;
	}

	mwindow->render->start_batches(&jobs);
}

void BatchRenderThread::stop_rendering()
{
	if(!is_rendering) return;
	mwindow->render->stop_operation();
	is_rendering = 0;
}

void BatchRenderThread::update_active(int number)
{
	gui->lock_window("BatchRenderThread::update_active");
	if(number >= 0)
	{
		current_job = number;
		rendering_job = number;
	}
	else
	{
		rendering_job = -1;
		is_rendering = 0;
	}
	gui->create_list(1);
	gui->unlock_window();
}

void BatchRenderThread::update_done(int number,
	int create_list,
	double elapsed_time)
{
	gui->lock_window("BatchRenderThread::update_done");
	if(number < 0)
	{
		gui->button_enable();
	}
	else
	{
		jobs.values[number]->enabled = 0;
		jobs.values[number]->elapsed = elapsed_time;
		if(create_list) gui->create_list(1);
	}
	gui->unlock_window();
}

void BatchRenderThread::move_batch(int src, int dst)
{
	BatchRenderJob *src_job = jobs.values[src];
	if(dst < 0) dst = jobs.total - 1;

	if(dst != src)
	{
		for(int i = src; i < jobs.total - 1; i++)
			jobs.values[i] = jobs.values[i + 1];
//		if(dst > src) dst--;
		for(int i = jobs.total - 1; i > dst; i--)
			jobs.values[i] = jobs.values[i - 1];
		jobs.values[dst] = src_job;
		gui->create_list(1);
	}
}

void BatchRenderThread::trap_hook(FILE *fp, void *vp)
{
	MWindow *mwindow = ((BatchRenderThread *)vp)->mwindow;
	fprintf(fp, "\nEDL:\n");
	mwindow->dump_edl(fp);
	fprintf(fp, "\nUNDO:\n");
	mwindow->dump_undo(fp);
	fprintf(fp, "\nEXE:\n");
	mwindow->dump_exe(fp);
}





BatchRenderGUI::BatchRenderGUI(MWindow *mwindow,
	BatchRenderThread *thread, int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Batch Render"),
	x, y, w, h, 50, 50, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

BatchRenderGUI::~BatchRenderGUI()
{
	lock_window("BatchRenderGUI::~BatchRenderGUI");
	delete format_tools;
	unlock_window();
}


void BatchRenderGUI::create_objects()
{
	lock_window("BatchRenderGUI::create_objects");
	mwindow->theme->get_batchrender_sizes(this, get_w(), get_h());
	create_list(0);

	int x = mwindow->theme->batchrender_x1;
	int y = 5;
	int x1 = x, x2 = get_w()/2 + 10; // mwindow->theme->batchrender_x2;
	int y1 = 5, y2 = 5;

// output file
	add_subwindow(output_path_title = new BC_Title(x1, y1, _("Output path:")));
	y1 += output_path_title->get_h() + mwindow->theme->widget_border;

	format_tools = new BatchFormat(mwindow, this, thread->get_current_asset());
	format_tools->set_w(get_w() / 2);
	format_tools->create_objects(x1, y1, 1, 1, 1, 1, 0, 1, 0, 0,
			&thread->get_current_job()->strategy, 0);

// input EDL
	add_subwindow(edl_path_title = new BC_Title(x2, y2, _("EDL Path:")));
	y2 += edl_path_title->get_h() + mwindow->theme->widget_border;

	x = x2;  y = y2;
	add_subwindow(edl_path_text = new BatchRenderEDLPath( thread,
		x, y, get_w()-x - 40, thread->get_current_edl()));
	x =  x2 + edl_path_text->get_w();
	add_subwindow(edl_path_browse = new BrowseButton(
		mwindow->theme, this, edl_path_text, x, y, thread->get_current_edl(),
		_("Input EDL"), _("Select an EDL to load:"), 0));
	y2 = y + edl_path_browse->get_h() + mwindow->theme->widget_border;

	x = x2;  y = y2;
	add_subwindow(update_selected_edl = new BatchRenderUpdateEDL(thread, x, y));
	y += update_selected_edl->get_h() + mwindow->theme->widget_border;
	add_subwindow(use_current_edl = new BatchRenderCurrentEDL(thread, x, y));
	y += use_current_edl->get_h() + mwindow->theme->widget_border;
	if( !mwindow->edl || !mwindow->edl->path[0] ) use_current_edl->disable();
	add_subwindow(new_batch = new BatchRenderNew(thread, x, y));
	x += new_batch->get_w() + mwindow->theme->widget_border;
	add_subwindow(delete_batch = new BatchRenderDelete(thread, x, y));
	x = x2;  y += delete_batch->get_h() + mwindow->theme->widget_border;
	add_subwindow(savelist_batch = new BatchRenderSaveList(thread, x, y));
	x += savelist_batch->get_w() + mwindow->theme->widget_border;
	add_subwindow(loadlist_batch = new BatchRenderLoadList(thread, x, y));
	y += loadlist_batch->get_h() + mwindow->theme->widget_border;
	add_subwindow(warning = new BatchRenderWarning(thread, x2, y));
	y2 = y + warning->get_h() + mwindow->theme->widget_border;
	if( y2 > y1 ) y1 = y2;
	x = mwindow->theme->batchrender_x1, y = y1;

	add_subwindow(list_title = new BC_Title(x, y, _("Batches to render:")));
	y += list_title->get_h() + mwindow->theme->widget_border;
	y1 = get_h();
	y1 -= 15 + BC_GenericButton::calculate_h() + mwindow->theme->widget_border;
	add_subwindow(batch_list = new BatchRenderList(thread, x, y,
		get_w() - x - 10, y1 - y));
	y += batch_list->get_h() + mwindow->theme->widget_border;

	add_subwindow(start_button = new BatchRenderStart(thread, x, y));
	x = get_w() / 2 - BC_GenericButton::calculate_w(this, _("Stop")) / 2;
	add_subwindow(stop_button = new BatchRenderStop(thread, x, y));
	x = get_w() - BC_GenericButton::calculate_w(this, _("Close")) - 10;
	add_subwindow(cancel_button = new BatchRenderCancel(thread, x, y));

	show_window(1);
	unlock_window();
}

void BatchRenderGUI::button_disable()
{
	new_batch->disable();
	delete_batch->disable();
	use_current_edl->disable();
	update_selected_edl->disable();
}

void BatchRenderGUI::button_enable()
{
	new_batch->enable();
	delete_batch->enable();
	if( mwindow->edl && mwindow->edl->path[0] )
		use_current_edl->enable();
	update_selected_edl->enable();
}

int BatchRenderGUI::resize_event(int w, int h)
{
	mwindow->session->batchrender_w = w;
	mwindow->session->batchrender_h = h;
	mwindow->theme->get_batchrender_sizes(this, w, h);

	int x = mwindow->theme->batchrender_x1;
	int y = 5;
	int x1 = x, x2 = get_w()/2 + 10; // mwindow->theme->batchrender_x2;
	int y1 = 5, y2 = 5;

// output file
	output_path_title->reposition_window(x1, y1);
	y1 += output_path_title->get_h() + mwindow->theme->widget_border;
	format_tools->reposition_window(x1, y1);

// input EDL
	x = x2, y = y2;
	edl_path_title->reposition_window(x, y);
	y += edl_path_title->get_h() + mwindow->theme->widget_border;
	edl_path_text->reposition_window(x, y, w - x - 40);
	x += edl_path_text->get_w();
	edl_path_browse->reposition_window(x, y);
	y2 = y + edl_path_browse->get_h() + mwindow->theme->widget_border;

	x = x2;  y = y2;
	update_selected_edl->reposition_window(x, y);
	y += update_selected_edl->get_h() + mwindow->theme->widget_border;
	use_current_edl->reposition_window(x, y);
	y += use_current_edl->get_h() + mwindow->theme->widget_border;
	new_batch->reposition_window(x, y);
	x += new_batch->get_w() + mwindow->theme->widget_border;
	delete_batch->reposition_window(x, y);

	x = x2;  y += delete_batch->get_h() + mwindow->theme->widget_border;
	savelist_batch->reposition_window(x, y);
	x += savelist_batch->get_w() + mwindow->theme->widget_border;
	loadlist_batch->reposition_window(x, y);
	y += loadlist_batch->get_h() + mwindow->theme->widget_border;

	y1 = 15 + BC_GenericButton::calculate_h() + mwindow->theme->widget_border;
	y2 = get_h() - y1 - batch_list->get_h();
	y2 -= list_title->get_h() + mwindow->theme->widget_border;

	x = mwindow->theme->batchrender_x1;  y = y2;
	list_title->reposition_window(x, y);
	y += list_title->get_h() + mwindow->theme->widget_border;
	batch_list->reposition_window(x, y, w - x - 10, h - y - y1);
	y += batch_list->get_h() + mwindow->theme->widget_border;

	start_button->reposition_window(x, y);
	x = w / 2 - stop_button->get_w() / 2;
	stop_button->reposition_window(x, y);
	x = w - cancel_button->get_w() - 10;
	cancel_button->reposition_window(x, y);
	return 1;
}

int BatchRenderGUI::translation_event()
{
	mwindow->session->batchrender_x = get_x();
	mwindow->session->batchrender_y = get_y();
	return 1;
}

int BatchRenderGUI::close_event()
{
// Stop batch rendering
	unlock_window();
	thread->stop_rendering();
	lock_window("BatchRenderGUI::close_event");
	set_done(1);
	return 1;
}

void BatchRenderGUI::create_list(int update_widget)
{
	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		list_columns[i].remove_all_objects();
	}

	for(int i = 0; i < thread->jobs.total; i++)
	{
		BatchRenderJob *job = thread->jobs.values[i];
		char string[BCTEXTLEN];
		BC_ListBoxItem *enabled = new BC_ListBoxItem(job->enabled ?
			(char*)"X" :
			(char*)" ");
		BC_ListBoxItem *item1 = new BC_ListBoxItem(job->asset->path);
		BC_ListBoxItem *item2 = new BC_ListBoxItem(job->edl_path);
		BC_ListBoxItem *item3;
		if(job->elapsed)
			item3 = new BC_ListBoxItem(
				Units::totext(string,
					job->elapsed,
					TIME_HMS2));
		else
			item3 = new BC_ListBoxItem(_("Unknown"));
		list_columns[0].append(enabled);
		list_columns[1].append(item1);
		list_columns[2].append(item2);
		list_columns[3].append(item3);
		if(i == thread->current_job)
		{
			enabled->set_selected(1);
			item1->set_selected(1);
			item2->set_selected(1);
			item3->set_selected(1);
		}
		if(i == thread->rendering_job)
		{
			enabled->set_color(RED);
			item1->set_color(RED);
			item2->set_color(RED);
			item3->set_color(RED);
		}
	}

	if(update_widget)
	{
		batch_list->update(list_columns,
						list_titles,
						thread->column_width,
						BATCHRENDER_COLUMNS,
						batch_list->get_xposition(),
						batch_list->get_yposition(),
						batch_list->get_highlighted_item(),  // Flat index of item cursor is over
						1,     // set all autoplace flags to 1
						1);
	}
}

void BatchRenderGUI::change_job()
{
	BatchRenderJob *job = thread->get_current_job();
	format_tools->update(job->asset, &job->strategy);
	edl_path_text->update(job->edl_path);
}








BatchFormat::BatchFormat(MWindow *mwindow,
			BatchRenderGUI *gui,
			Asset *asset)
 : FormatTools(mwindow, gui, asset)
{
	this->gui = gui;
	this->mwindow = mwindow;
}

BatchFormat::~BatchFormat()
{
}


int BatchFormat::handle_event()
{
	gui->create_list(1);
	return 1;
}











BatchRenderEDLPath::BatchRenderEDLPath(BatchRenderThread *thread,
	int x,
	int y,
	int w,
	char *text)
 : BC_TextBox(x,
		y,
		w,
		1,
		text)
{
	this->thread = thread;
}


int BatchRenderEDLPath::handle_event()
{
// Suggestions
	calculate_suggestions(thread->file_entries);

	strcpy(thread->get_current_edl(), get_text());
	thread->gui->create_list(1);
	return 1;
}






BatchRenderNew::BatchRenderNew(BatchRenderThread *thread,
	int x,
	int y)
 : BC_GenericButton(x, y, _("New"))
{
	this->thread = thread;
}

int BatchRenderNew::handle_event()
{
	thread->new_job();
	return 1;
}

BatchRenderDelete::BatchRenderDelete(BatchRenderThread *thread,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->thread = thread;
}

int BatchRenderDelete::handle_event()
{
	thread->delete_job();
	return 1;
}



BatchRenderSaveList::BatchRenderSaveList(BatchRenderThread *thread,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Save List"))
{
	this->thread = thread;
	set_tooltip(_("Save a Batch Render List"));
	gui = 0;
	startup_lock = new Mutex("BatchRenderSaveList::startup_lock");
}

BatchRenderSaveList::~BatchRenderSaveList()
{
	startup_lock->lock("BatchRenderSaveList::~BrowseButton");
	if(gui)
	{
		gui->lock_window();
		gui->set_done(1);
		gui->unlock_window();
	}
	startup_lock->unlock();
	Thread::join();
	delete startup_lock;
}

int BatchRenderSaveList::handle_event()
{
	if(Thread::running())
	{
		if(gui)
		{
			gui->lock_window();
			gui->raise_window();
			gui->unlock_window();
		}
		return 1;
	}
	startup_lock->lock("BatchRenderSaveList::handle_event 1");
	Thread::start();
	startup_lock->lock("BatchRenderSaveList::handle_event 2");
	startup_lock->unlock();
	return 1;
}

void BatchRenderSaveList::run()
{
	char default_path[BCTEXTLEN];
	sprintf(default_path, "~");
	BC_FileBox filewindow(100, 100,
			this->thread->mwindow->defaults->get("DEFAULT_BATCHLOADPATH", default_path),
			_("Save Batch Render List"), _("Enter a Batch Render filename to save as:"),
			0, 0, 0, 0);
	gui = &filewindow;

	startup_lock->unlock();
	filewindow.create_objects();

	int result2 = filewindow.run_window();

	if(!result2)
	{
		this->thread->save_jobs(filewindow.get_submitted_path());
		this->thread->mwindow->defaults->update("DEFAULT_BATCHLOADPATH", filewindow.get_submitted_path());
	}

	this->thread->gui->flush();
	startup_lock->lock("BatchRenderLoadList::run");
	gui = 0;
	startup_lock->unlock();
}

int BatchRenderSaveList::keypress_event() {
	if (get_keypress() == 's' ||
	    get_keypress() == 'S') return handle_event();
	return 0;
}




BatchRenderLoadList::BatchRenderLoadList(BatchRenderThread *thread,
	int x,
	int y)
  : BC_GenericButton(x, y, _("Load List")),
    Thread()
{
	this->thread = thread;
	set_tooltip(_("Load a previously saved Batch Render List"));
	gui = 0;
	startup_lock = new Mutex("BatchRenderLoadList::startup_lock");
}

BatchRenderLoadList::~BatchRenderLoadList()
{
	startup_lock->lock("BatchRenderLoadList::~BrowseButton");
	if(gui)
	{
		gui->lock_window();
		gui->set_done(1);
		gui->unlock_window();
	}
	startup_lock->unlock();
	Thread::join();
	delete startup_lock;
}

int BatchRenderLoadList::handle_event()
{
	if(Thread::running())
	{
		if(gui)
		{
			gui->lock_window();
			gui->raise_window();
			gui->unlock_window();
		}
		return 1;
	}
	startup_lock->lock("BatchRenderLoadList::handle_event 1");
	Thread::start();
	startup_lock->lock("BatchRenderLoadList::handle_event 2");
	startup_lock->unlock();
	return 1;
}

void BatchRenderLoadList::run()
{
	char default_path[BCTEXTLEN];
	sprintf(default_path, "~");
	BC_FileBox filewindow(100,
			      100,
			      this->thread->mwindow->defaults->get("DEFAULT_BATCHLOADPATH", default_path),
			      _("Load Batch Render List"),
			      _("Enter a Batch Render filename to load from:"),
			      0,
			      0,
			      0,
			      0);

	gui = &filewindow;

	startup_lock->unlock();
	filewindow.create_objects();

	int result2 = filewindow.run_window();

	if(!result2)
	{
		this->thread->load_jobs(filewindow.get_submitted_path(),this->thread->mwindow->preferences);
		this->thread->gui->create_list(1);
		this->thread->mwindow->defaults->update("DEFAULT_BATCHLOADPATH", filewindow.get_submitted_path());
	}

	this->thread->gui->flush();
	startup_lock->lock("BatchRenderLoadList::run");
	gui = 0;
	startup_lock->unlock();
}

int BatchRenderLoadList::keypress_event() {
	if (get_keypress() == 'o' ||
	    get_keypress() == 'O') return handle_event();
	return 0;
}

BatchRenderCurrentEDL::BatchRenderCurrentEDL(BatchRenderThread *thread,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Use Current EDL"))
{
	this->thread = thread;
}

int BatchRenderCurrentEDL::handle_event()
{
	thread->use_current_edl();
	return 1;
}

BatchRenderUpdateEDL::BatchRenderUpdateEDL(BatchRenderThread *thread,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Save to EDL Path"))
{
	this->thread = thread;
}

int BatchRenderUpdateEDL::handle_event()
{
	thread->update_selected_edl();
	return 1;
}




BatchRenderList::BatchRenderList(BatchRenderThread *thread,
	int x,
	int y,
	int w,
	int h)
 : BC_ListBox(x,
 	y,
	w,
	h,
	LISTBOX_TEXT,
	thread->gui->list_columns,
	list_titles,
	thread->column_width,
	BATCHRENDER_COLUMNS,
	0,
	0,
	LISTBOX_SINGLE,
	ICON_LEFT,
	1)
{
	this->thread = thread;
	dragging_item = 0;
	set_process_drag(0);
}

int BatchRenderList::handle_event()
{
	return 1;
}

int BatchRenderList::selection_changed()
{
	thread->current_job = get_selection_number(0, 0);
	thread->gui->change_job();
	if(get_cursor_x() < thread->column_width[0])
	{
		BatchRenderJob *job = thread->get_current_job();
		job->enabled = !job->enabled;
		thread->gui->create_list(1);
	}
	return 1;
}

int BatchRenderList::column_resize_event()
{
	for(int i = 0; i < BATCHRENDER_COLUMNS; i++)
	{
		thread->column_width[i] = get_column_width(i);
	}
	return 1;
}

int BatchRenderList::drag_start_event()
{
	if(BC_ListBox::drag_start_event())
	{
		dragging_item = 1;
		return 1;
	}

	return 0;
}

int BatchRenderList::drag_motion_event()
{
	if(BC_ListBox::drag_motion_event())
	{
		return 1;
	}
	return 0;
}

int BatchRenderList::drag_stop_event()
{
	if(dragging_item)
	{
		int src = get_selection_number(0, 0);
		int dst = get_highlighted_item();
		if(src != dst)
		{
			thread->move_batch(src, dst);
		}
		BC_ListBox::drag_stop_event();
	}
	return 0;
}













BatchRenderStart::BatchRenderStart(BatchRenderThread *thread,
	int x,
	int y)
 : BC_GenericButton(x,
 	y,
	_("Start"))
{
	this->thread = thread;
}

int BatchRenderStart::handle_event()
{
	thread->start_rendering();
	return 1;
}

BatchRenderStop::BatchRenderStop(BatchRenderThread *thread,
	int x,
	int y)
 : BC_GenericButton(x,
 	y,
	_("Stop"))
{
	this->thread = thread;
}

int BatchRenderStop::handle_event()
{
	unlock_window();
	thread->stop_rendering();
	lock_window("BatchRenderStop::handle_event");
	return 1;
}


BatchRenderWarning::BatchRenderWarning(BatchRenderThread *thread, int x, int y)
 : BC_CheckBox(x, y, thread->warn, _("warn if jobs/session mismatched"))
{
	this->thread = thread;
}

int BatchRenderWarning::handle_event()
{
	thread->warn = get_value();
	return 1;
}


BatchRenderCancel::BatchRenderCancel(BatchRenderThread *thread,
	int x,
	int y)
 : BC_GenericButton(x,
 	y,
	_("Close"))
{
	this->thread = thread;
}

int BatchRenderCancel::handle_event()
{
	unlock_window();
	thread->stop_rendering();
	lock_window("BatchRenderCancel::handle_event");
	thread->gui->set_done(1);
	return 1;
}

int BatchRenderCancel::keypress_event()
{
	if(get_keypress() == ESC)
	{
		unlock_window();
		thread->stop_rendering();
		lock_window("BatchRenderCancel::keypress_event");
		thread->gui->set_done(1);
		return 1;
	}
	return 0;
}

