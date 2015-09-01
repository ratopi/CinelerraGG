#include "asset.h"
#include "bdcreate.h"
#include "edl.h"
#include "edit.h"
#include "edits.h"
#include "edlsession.h"
#include "file.inc"
#include "filexml.h"
#include "format.inc"
#include "keyframe.h"
#include "labels.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "pluginset.h"
#include "track.h"
#include "tracks.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>

// BD Creation

const int64_t CreateBD_Thread::BD_SIZE = 25000000000;
const int CreateBD_Thread::BD_STREAMS = 1;
const int CreateBD_Thread::BD_WIDTH = 1920;
const int CreateBD_Thread::BD_HEIGHT = 1080;
const double CreateBD_Thread::BD_ASPECT_WIDTH = 4.;
const double CreateBD_Thread::BD_ASPECT_HEIGHT = 3.;
const double CreateBD_Thread::BD_WIDE_ASPECT_WIDTH = 16.;
const double CreateBD_Thread::BD_WIDE_ASPECT_HEIGHT = 9.;
const double CreateBD_Thread::BD_FRAMERATE = 24000. / 1001.;
//const int CreateBD_Thread::BD_MAX_BITRATE = 40000000;
const int CreateBD_Thread::BD_MAX_BITRATE = 8000000;
const int CreateBD_Thread::BD_CHANNELS = 2;
const int CreateBD_Thread::BD_WIDE_CHANNELS = 6;
const double CreateBD_Thread::BD_SAMPLERATE = 48000;
const double CreateBD_Thread::BD_KAUDIO_RATE = 224;


CreateBD_MenuItem::CreateBD_MenuItem(MWindow *mwindow)
 : BC_MenuItem(_("BD Render..."), _("Ctrl-d"), 'd')
{
	set_ctrl(1); 
	this->mwindow = mwindow;
}

int CreateBD_MenuItem::handle_event()
{
	mwindow->create_bd->start();
	return 1;
}


CreateBD_Thread::CreateBD_Thread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->gui = 0;
	this->use_deinterlace = 0;
	this->use_inverse_telecine = 0;
	this->use_scale = 0;
	this->use_resize_tracks = 0;
	this->use_histogram = 0;
	this->use_wide_audio = 0;
	this->use_wide_aspect = 0;
	this->use_label_chapters = 0;
}

CreateBD_Thread::~CreateBD_Thread()
{
	close_window();
}

int CreateBD_Thread::create_bd_jobs(ArrayList<BatchRenderJob*> *jobs,
	const char *tmp_path, const char *asset_title)
{
	EDL *edl = mwindow->edl;
	if( !edl || !edl->session ) {
		char msg[BCTEXTLEN];
                sprintf(msg, _("No EDL/Session"));
                MainError::show_error(msg);
                return 1;
        }
	EDLSession *session = edl->session;

	double total_length = edl->tracks->total_length();
        if( total_length <= 0 ) {
		char msg[BCTEXTLEN];
                sprintf(msg, _("No content: %s"), asset_title);
                MainError::show_error(msg);
                return 1;
        }

	char asset_dir[BCTEXTLEN];
	sprintf(asset_dir, "%s/%s", tmp_path, asset_title);

	if( mkdir(asset_dir, 0777) ) {
		char err[BCTEXTLEN], msg[BCTEXTLEN];
		strerror_r(errno, err, sizeof(err));
		sprintf(msg, _("Unable to create directory: %s\n-- %s"), asset_dir, err);
		MainError::show_error(msg);
		return 1;
	}

	double old_samplerate = session->sample_rate;
	double old_framerate = session->frame_rate;

        session->video_channels = BD_STREAMS;
        session->video_tracks = BD_STREAMS;
// use session framerate
//      session->frame_rate = BD_FRAMERATE;
        session->output_w = BD_WIDTH;
        session->output_h = BD_HEIGHT;
        session->aspect_w = use_wide_aspect ? BD_WIDE_ASPECT_WIDTH : BD_ASPECT_WIDTH;
        session->aspect_h = use_wide_aspect ? BD_WIDE_ASPECT_HEIGHT : BD_ASPECT_HEIGHT;
        session->sample_rate = BD_SAMPLERATE;
        session->audio_channels = session->audio_tracks =
		use_wide_audio ? BD_WIDE_CHANNELS : BD_CHANNELS;

	char script_filename[BCTEXTLEN];
	sprintf(script_filename, "%s/bd.sh", asset_dir);
	int fd = open(script_filename, O_WRONLY+O_CREAT+O_TRUNC, 0755);
	FILE *fp = fdopen(fd, "w");
	if( !fp ) {
		char err[BCTEXTLEN], msg[BCTEXTLEN];
		strerror_r(errno, err, sizeof(err));
		sprintf(msg, _("Unable to save: %s\n-- %s"), script_filename, err);
		MainError::show_error(msg);
		return 1;
	}
	char exe_path[BCTEXTLEN];
	get_exe_path(exe_path);
	fprintf(fp,"#!/bin/bash -ex\n");
	fprintf(fp,"PATH=$PATH:%s\n",exe_path);
	fprintf(fp,"mkdir -p $1/udfs\n");
	fprintf(fp,"sz=`du -sb $1/bd.m2ts | sed -e 's/[ \t].*//'`\n");
	fprintf(fp,"blks=$((sz/2048 + 4096))\n");
	fprintf(fp,"mkudffs $1/bd.udfs $blks\n");
	fprintf(fp,"mount -o loop $1/bd.udfs $1/udfs\n");
	fprintf(fp,"bdwrite $1/udfs $1/bd.m2ts\n");
	fprintf(fp,"umount $1/udfs\n");
	fprintf(fp,"echo To burn bluray, load writable media and run:\n");
	fprintf(fp,"echo growisofs -dvd-compat -Z /dev/bd=$1/bd.udfs\n");
	fprintf(fp,"\n");
	fclose(fp);

	if( use_wide_audio ) {
        	session->audio_channels = session->audio_tracks = BD_WIDE_CHANNELS;
		session->achannel_positions[0] = 90;
		session->achannel_positions[1] = 150;
		session->achannel_positions[2] = 30;
		session->achannel_positions[3] = 210;
		session->achannel_positions[4] = 330;
		session->achannel_positions[5] = 270;
		if( edl->tracks->recordable_audio_tracks() == BD_WIDE_CHANNELS )
			mwindow->remap_audio(MWindow::AUDIO_1_TO_1);
	}
	else {
        	session->audio_channels = session->audio_tracks = BD_CHANNELS;
		session->achannel_positions[0] = 180;
		session->achannel_positions[1] = 0;
		if( edl->tracks->recordable_audio_tracks() == BD_WIDE_CHANNELS )
			mwindow->remap_audio(MWindow::AUDIO_5_1_TO_2);
	}

	double new_samplerate = session->sample_rate;
	double new_framerate = session->frame_rate;
	edl->rechannel();
	edl->resample(old_samplerate, new_samplerate, TRACK_AUDIO);
	edl->resample(old_framerate, new_framerate, TRACK_VIDEO);

	int64_t aud_size = ((BD_KAUDIO_RATE * total_length)/8 + 1000-1) * 1000;
	int64_t vid_size = BD_SIZE*0.96 - aud_size;
	int64_t vid_bitrate = (vid_size * 8) / total_length;
	vid_bitrate /= 1000;  vid_bitrate *= 1000;
	if( vid_bitrate > BD_MAX_BITRATE ) vid_bitrate = BD_MAX_BITRATE;

	char xml_filename[BCTEXTLEN];
	sprintf(xml_filename, "%s/bd.xml", asset_dir);
        FileXML xml_file;
        edl->save_xml(&xml_file, xml_filename, 0, 0);
        xml_file.terminate_string();
        if( xml_file.write_to_file(xml_filename) ) {
		char msg[BCTEXTLEN];
		sprintf(msg, _("Unable to save: %s"), xml_filename);
		MainError::show_error(msg);
		return 1;
	}

	BatchRenderJob *job = new BatchRenderJob(mwindow->preferences);
	jobs->append(job);
	strcpy(&job->edl_path[0], xml_filename);
	Asset *asset = job->asset;

	asset->layers = BD_STREAMS;
	asset->frame_rate = session->frame_rate;
	asset->width = session->output_w;
	asset->height = session->output_h;
	asset->aspect_ratio = session->aspect_w / session->aspect_h;

	char option_path[BCTEXTLEN];
	sprintf(&asset->path[0],"%s/bd.m2ts", asset_dir);
	asset->format = FILE_FFMPEG;
	strcpy(asset->fformat, "m2ts");

	asset->audio_data = 1;
	strcpy(asset->acodec, "bluray.m2ts");
	FFMPEG::set_option_path(option_path, "audio/%s", asset->acodec);
	FFMPEG::load_options(option_path, asset->ff_audio_options,
			 sizeof(asset->ff_audio_options));
	asset->ff_audio_bitrate = BD_KAUDIO_RATE * 1000;

      	asset->video_data = 1;
	strcpy(asset->vcodec, "bluray.m2ts");
	FFMPEG::set_option_path(option_path, "video/%s", asset->vcodec);
	FFMPEG::load_options(option_path, asset->ff_video_options,
		 sizeof(asset->ff_video_options));
	asset->ff_video_bitrate = vid_bitrate;
	asset->ff_video_quality = 0;

	int len = strlen(asset->ff_video_options);
	char *cp = asset->ff_video_options + len;
	snprintf(cp, sizeof(asset->ff_video_options)-len-1,
		"aspect %.5f\n", asset->aspect_ratio);

	job = new BatchRenderJob(mwindow->preferences);
	jobs->append(job);
	job->edl_path[0] = '@';
	strcpy(&job->edl_path[1], script_filename);
	strcpy(&job->asset->path[0], asset_dir);

	return 0;
}

void CreateBD_Thread::handle_close_event(int result)
{
	if( result ) return;
	mwindow->batch_render->load_defaults(mwindow->defaults);
        mwindow->undo->update_undo_before();
	KeyFrame keyframe;  char data[BCTEXTLEN];
	if( use_deinterlace ) {
		sprintf(data,"<DEINTERLACE MODE=1>");
		keyframe.set_data(data);
		insert_video_plugin("Deinterlace", &keyframe);
	}
	if( use_inverse_telecine ) {
		sprintf(data,"<IVTC FRAME_OFFSET=0 FIRST_FIELD=0 "
			"AUTOMATIC=1 AUTO_THRESHOLD=2.0e+00 PATTERN=2>");
		keyframe.set_data(data);
		insert_video_plugin("Inverse Telecine", &keyframe);
	}
	if( use_scale ) {
		sprintf(data,"<PHOTOSCALE WIDTH=%d HEIGHT=%d USE_FILE=1>", BD_WIDTH, BD_HEIGHT);
		keyframe.set_data(data);
		insert_video_plugin("Auto Scale", &keyframe);
	}
	if( use_resize_tracks )
		resize_tracks();
	if( use_histogram ) {
#if 0
		sprintf(data, "<HISTOGRAM OUTPUT_MIN_0=0 OUTPUT_MAX_0=1 "
			"OUTPUT_MIN_1=0 OUTPUT_MAX_1=1 "
			"OUTPUT_MIN_2=0 OUTPUT_MAX_2=1 "
			"OUTPUT_MIN_3=0 OUTPUT_MAX_3=1 "
			"AUTOMATIC=0 THRESHOLD=9.0-01 PLOT=0 SPLIT=0>"
			"<POINTS></POINTS><POINTS></POINTS><POINTS></POINTS>"
			"<POINTS><POINT X=6.0e-02 Y=0>"
				"<POINT X=9.4e-01 Y=1></POINTS>");
#else
		sprintf(data, "<HISTOGRAM AUTOMATIC=0 THRESHOLD=1.0e-01 "
			"PLOT=0 SPLIT=0 W=440 H=500 PARADE=0 MODE=3 "
			"LOW_OUTPUT_0=0 HIGH_OUTPUT_0=1 LOW_INPUT_0=0 HIGH_INPUT_0=1 GAMMA_0=1 "
			"LOW_OUTPUT_1=0 HIGH_OUTPUT_1=1 LOW_INPUT_1=0 HIGH_INPUT_1=1 GAMMA_1=1 "
			"LOW_OUTPUT_2=0 HIGH_OUTPUT_2=1 LOW_INPUT_2=0 HIGH_INPUT_2=1 GAMMA_2=1 "
			"LOW_OUTPUT_3=0 HIGH_OUTPUT_3=1 LOW_INPUT_3=0.044 HIGH_INPUT_3=0.956 "
			"GAMMA_3=1>");
#endif
		keyframe.set_data(data);
		insert_video_plugin("Histogram", &keyframe);
	}
	create_bd_jobs(&mwindow->batch_render->jobs, tmp_path, asset_title);
	mwindow->save_backup();
	mwindow->undo->update_undo_after(_("create bd"), LOAD_ALL);
	mwindow->resync_guis();
	mwindow->batch_render->handle_close_event(0);
	mwindow->batch_render->start();
}

BC_Window* CreateBD_Thread::new_gui()
{
	memset(tmp_path,0,sizeof(tmp_path));
	strcpy(tmp_path,"/tmp");
	memset(asset_title,0,sizeof(asset_title));
	time_t dt;      time(&dt);
	struct tm dtm;  localtime_r(&dt, &dtm);
	sprintf(asset_title, "bd_%02d%02d%02d-%02d%02d%02d",
		dtm.tm_year+1900, dtm.tm_mon+1, dtm.tm_mday,
		dtm.tm_hour, dtm.tm_min, dtm.tm_sec);
	use_deinterlace = 0;
	use_inverse_telecine = 0;
	use_scale = 0;
	use_resize_tracks = 0;
	use_histogram = 0;
	use_wide_audio = 0;
	use_wide_aspect = 0;
	use_label_chapters = 0;
	option_presets();
        int scr_x = mwindow->gui->get_screen_x(0, -1);
        int scr_w = mwindow->gui->get_screen_w(0, -1);
        int scr_h = mwindow->gui->get_screen_h(0, -1);
        int w = 500, h = 250;
	int x = scr_x + scr_w/2 - w/2, y = scr_h/2 - h/2;

	gui = new CreateBD_GUI(this, x, y, w, h);
	gui->create_objects();
	return gui;
}


CreateBD_OK::CreateBD_OK(CreateBD_GUI *gui, int x, int y)
 : BC_OKButton(x, y)
{
        this->gui = gui;
        set_tooltip(_("end setup, start batch render"));
}

CreateBD_OK::~CreateBD_OK()
{
}

int CreateBD_OK::button_press_event()
{
        if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
                gui->set_done(0);
                return 1;
        }
        return 0;
}

int CreateBD_OK::keypress_event()
{
        return 0;
}


CreateBD_Cancel::CreateBD_Cancel(CreateBD_GUI *gui, int x, int y)
 : BC_CancelButton(x, y)
{
        this->gui = gui;
}

CreateBD_Cancel::~CreateBD_Cancel()
{
}

int CreateBD_Cancel::button_press_event()
{
        if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
                gui->set_done(1);
                return 1;
        }
        return 0;
}


CreateBD_DiskSpace::CreateBD_DiskSpace(CreateBD_GUI *gui, int x, int y)
 : BC_Title(x, y, "", MEDIUMFONT, GREEN)
{
        this->gui = gui;
}

CreateBD_DiskSpace::~CreateBD_DiskSpace()
{
}

int64_t CreateBD_DiskSpace::tmp_path_space()
{
	const char *path = gui->tmp_path->get_text();
	if( access(path,R_OK+W_OK) ) return 0;
	struct statfs sfs;
	if( statfs(path, &sfs) ) return 0;
	return (int64_t)sfs.f_bsize * sfs.f_bfree;
}

void CreateBD_DiskSpace::update()
{
//	gui->disk_space->set_color(get_bg_color());
	int64_t disk_space = tmp_path_space();
	int color = disk_space<gui->needed_disk_space ? RED : GREEN;
	static const char *suffix[] = { "", "KB", "MB", "GB", "TB", "PB" };
	int i = 0;
	for( int64_t space=disk_space; i<5 && (space/=1000)>0; disk_space=space, ++i );
	char text[BCTEXTLEN];
	sprintf(text, "%s" _LDv(3) "%s", _("disk space: "), disk_space, suffix[i]);
	gui->disk_space->BC_Title::update(text);
	gui->disk_space->set_color(color);
}

CreateBD_TmpPath::CreateBD_TmpPath(CreateBD_GUI *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, -(int)sizeof(gui->thread->tmp_path),
		gui->thread->tmp_path, 1, MEDIUMFONT)
{
        this->gui = gui;
}

CreateBD_TmpPath::~CreateBD_TmpPath()
{
}

int CreateBD_TmpPath::handle_event()
{
	gui->disk_space->update();
        return 1;
}


CreateBD_AssetTitle::CreateBD_AssetTitle(CreateBD_GUI *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, 0, gui->thread->asset_title, 1, MEDIUMFONT)
{
        this->gui = gui;
}

CreateBD_AssetTitle::~CreateBD_AssetTitle()
{
}


CreateBD_Deinterlace::CreateBD_Deinterlace(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_deinterlace, _("Deinterlace"))
{
	this->gui = gui;
}

CreateBD_Deinterlace::~CreateBD_Deinterlace()
{
}

int CreateBD_Deinterlace::handle_event()
{
	if( get_value() ) {
		gui->need_inverse_telecine->set_value(0);
		gui->thread->use_inverse_telecine = 0;
	}
	return BC_CheckBox::handle_event();
}


CreateBD_InverseTelecine::CreateBD_InverseTelecine(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_inverse_telecine, _("Inverse Telecine"))
{
	this->gui = gui;
}

CreateBD_InverseTelecine::~CreateBD_InverseTelecine()
{
}

int CreateBD_InverseTelecine::handle_event()
{
	if( get_value() ) {
		gui->need_deinterlace->set_value(0);
		gui->thread->use_deinterlace = 0;
	}
	return BC_CheckBox::handle_event();
}


CreateBD_Scale::CreateBD_Scale(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_scale, _("Scale"))
{
	this->gui = gui;
}

CreateBD_Scale::~CreateBD_Scale()
{
}


CreateBD_ResizeTracks::CreateBD_ResizeTracks(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_resize_tracks, _("Resize Tracks"))
{
	this->gui = gui;
}

CreateBD_ResizeTracks::~CreateBD_ResizeTracks()
{
}


CreateBD_Histogram::CreateBD_Histogram(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_histogram, _("Histogram"))
{
	this->gui = gui;
}

CreateBD_Histogram::~CreateBD_Histogram()
{
}

CreateBD_LabelChapters::CreateBD_LabelChapters(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_label_chapters, _("Chapters at Labels"))
{
	this->gui = gui;
}

CreateBD_LabelChapters::~CreateBD_LabelChapters()
{
}

CreateBD_WideAudio::CreateBD_WideAudio(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_wide_audio, _("Audio 5.1"))
{
	this->gui = gui;
}

CreateBD_WideAudio::~CreateBD_WideAudio()
{
}

CreateBD_WideAspect::CreateBD_WideAspect(CreateBD_GUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->thread->use_wide_aspect, _("Aspect 16x9"))
{
	this->gui = gui;
}

CreateBD_WideAspect::~CreateBD_WideAspect()
{
}



CreateBD_GUI::CreateBD_GUI(CreateBD_Thread *thread, int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Create BD"), x, y, w, h, 50, 50, 1, 0, 1)
{
	this->thread = thread;
	at_x = at_y = tmp_x = tmp_y = 0;
	ok_x = ok_y = ok_w = ok_h = 0;
	cancel_x = cancel_y = cancel_w = cancel_h = 0;
	asset_title = 0;
	tmp_path = 0;
	disk_space = 0;
	needed_disk_space = 100e9;
	need_deinterlace = 0;
	need_inverse_telecine = 0;
	need_scale = 0;
	need_resize_tracks = 0;
	need_histogram = 0;
	need_wide_audio = 0;
	need_wide_aspect = 0;
	need_label_chapters = 0;
	ok = 0;
	cancel = 0;
}

CreateBD_GUI::~CreateBD_GUI()
{
}

void CreateBD_GUI::create_objects()
{
	lock_window("CreateBD_GUI::create_objects");
	int pady = BC_TextBox::calculate_h(this, MEDIUMFONT, 0, 1) + 5;
	int padx = BC_Title::calculate_w(this, (char*)"X", MEDIUMFONT);
	int x = padx/2, y = pady/2;
	BC_Title *title = new BC_Title(x, y, _("Title:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	at_x = x + title->get_w();  at_y = y;
	asset_title = new CreateBD_AssetTitle(this, at_x, at_y, get_w()-at_x-10);
	add_subwindow(asset_title);
	y += title->get_h() + pady/2;
	title = new BC_Title(x, y, _("tmp path:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);
	tmp_x = x + title->get_w();  tmp_y = y;
	tmp_path = new CreateBD_TmpPath(this, tmp_x, tmp_y,  get_w()-tmp_x-10);
	add_subwindow(tmp_path);
	y += title->get_h() + pady/2;
	disk_space = new CreateBD_DiskSpace(this, x, y);
	add_subwindow(disk_space);
	disk_space->update();
	y += disk_space->get_h() + pady/2;
	need_deinterlace = new CreateBD_Deinterlace(this, x, y);
	add_subwindow(need_deinterlace);
	int x1 = x + 150, x2 = x1 + 150;
	need_inverse_telecine = new CreateBD_InverseTelecine(this, x1, y);
	add_subwindow(need_inverse_telecine);
	y += need_deinterlace->get_h() + pady/2;
	need_scale = new CreateBD_Scale(this, x, y);
	add_subwindow(need_scale);
	need_wide_audio = new CreateBD_WideAudio(this, x1, y);
	add_subwindow(need_wide_audio);
	need_resize_tracks = new CreateBD_ResizeTracks(this, x2, y);
	add_subwindow(need_resize_tracks);
	y += need_scale->get_h() + pady/2;
	need_histogram = new CreateBD_Histogram(this, x, y);
	add_subwindow(need_histogram);
	need_wide_aspect = new CreateBD_WideAspect(this, x1, y);
	add_subwindow(need_wide_aspect);
//	need_label_chapters = new CreateBD_LabelChapters(this, x2, y);
//	add_subwindow(need_label_chapters);
	ok_w = BC_OKButton::calculate_w();
	ok_h = BC_OKButton::calculate_h();
	ok_x = 10;
	ok_y = get_h() - ok_h - 10;
	ok = new CreateBD_OK(this, ok_x, ok_y);
	add_subwindow(ok);
	cancel_w = BC_CancelButton::calculate_w();
	cancel_h = BC_CancelButton::calculate_h();
	cancel_x = get_w() - cancel_w - 10,
	cancel_y = get_h() - cancel_h - 10;
	cancel = new CreateBD_Cancel(this, cancel_x, cancel_y);
	add_subwindow(cancel);
	show_window();
	unlock_window();
}

int CreateBD_GUI::resize_event(int w, int h)
{
	asset_title->reposition_window(at_x, at_y, get_w()-at_x-10);
	tmp_path->reposition_window(tmp_x, tmp_y,  get_w()-tmp_x-10);
        ok_y = h - ok_h - 10;
        ok->reposition_window(ok_x, ok_y);
	cancel_x = w - cancel_w - 10,
	cancel_y = h - cancel_h - 10;
        cancel->reposition_window(cancel_x, cancel_y);
	return 0;
}

int CreateBD_GUI::translation_event()
{
	return 1;
}

int CreateBD_GUI::close_event()
{
        set_done(1);
        return 1;
}

int CreateBD_Thread::
insert_video_plugin(const char *title, KeyFrame *default_keyframe)
{
	Tracks *tracks = mwindow->edl->tracks;
	for( Track *vtrk=tracks->first; vtrk; vtrk=vtrk->next ) {
		if( vtrk->data_type != TRACK_VIDEO ) continue;
		if( !vtrk->record ) continue;
		vtrk->expand_view = 1;
		PluginSet *plugin_set = new PluginSet(mwindow->edl, vtrk);
		vtrk->plugin_set.append(plugin_set);
		Edits *edits = vtrk->edits;
		for( Edit *edit=edits->first; edit; edit=edit->next ) {
			plugin_set->insert_plugin(title,
				edit->startproject, edit->length,
				PLUGIN_STANDALONE, 0, default_keyframe, 0);
		}
		vtrk->optimize();
	}
        return 0;
}

int CreateBD_Thread::
resize_tracks()
{
        Tracks *tracks = mwindow->edl->tracks;
#if 0
	int max_w = 0, max_h = 0;
        for( Track *vtrk=tracks->first; vtrk; vtrk=vtrk->next ) {
		if( vtrk->data_type != TRACK_VIDEO ) continue;
		if( !vtrk->record ) continue;
		Edits *edits = vtrk->edits;
		for( Edit *edit=edits->first; edit; edit=edit->next ) {
			Indexable *indexable = edit->get_source();
			int w = indexable->get_w();
			if( w > max_w ) max_w = w;
			int h = indexable->get_h();
			if( h > max_h ) max_h = h;
		}
        }
#endif
        for( Track *vtrk=tracks->first; vtrk; vtrk=vtrk->next ) {
                if( vtrk->data_type != TRACK_VIDEO ) continue;
                if( !vtrk->record ) continue;
		vtrk->track_w = BD_WIDTH; // max_w;
		vtrk->track_h = BD_HEIGHT; // max_h;
	}
        return 0;
}

int CreateBD_Thread::
option_presets()
{
	if( !mwindow->edl ) return 1;
        Tracks *tracks = mwindow->edl->tracks;
	int max_w = 0, max_h = 0;
	int has_deinterlace = 0, has_scale = 0;
        for( Track *trk=tracks->first; trk; trk=trk->next ) {
		if( !trk->record ) continue;
		Edits *edits = trk->edits;
		switch( trk->data_type ) {
		case TRACK_VIDEO:
			for( Edit *edit=edits->first; edit; edit=edit->next ) {
				Indexable *indexable = edit->get_source();
				int w = indexable->get_w();
				if( w > max_w ) max_w = w;
				if( w != BD_WIDTH ) use_scale = 1;
				int h = indexable->get_h();
				if( h > max_h ) max_h = h;
				if( h != BD_HEIGHT ) use_scale = 1;
			}
			for( int i=0; i<trk->plugin_set.size(); ++i ) {
				for(Plugin *plugin = (Plugin*)trk->plugin_set[i]->first;
                                                plugin;
                                                plugin = (Plugin*)plugin->next) {
					if( !strcmp(plugin->title, "Deinterlace") )
						has_deinterlace = 1;
					if( !strcmp(plugin->title, "Auto Scale") ||
					    !strcmp(plugin->title, "Scale") )
						has_scale = 1;
				}
			}
			break;
		}
        }
	if( has_scale )
		use_scale = 0;
	if( use_scale ) {
		if( max_w != BD_WIDTH ) use_resize_tracks = 1;
		if( max_h != BD_HEIGHT ) use_resize_tracks = 1;
	}
        for( Track *trk=tracks->first; trk && !use_resize_tracks; trk=trk->next ) {
		if( !trk->record ) continue;
		switch( trk->data_type ) {
		case TRACK_VIDEO:
			if( trk->track_w != max_w ) use_resize_tracks = 1;
			if( trk->track_h != max_h ) use_resize_tracks = 1;
			break;
		}
        }
	if( !has_deinterlace && max_h > 2*BD_HEIGHT ) use_deinterlace = 1;
	// Labels *labels = mwindow->edl->labels;
	// use_label_chapters = labels && labels->first ? 1 : 0;
	float w, h;
	MWindow::create_aspect_ratio(w, h, max_w, max_h);
	if( w == BD_WIDE_ASPECT_WIDTH && h == BD_WIDE_ASPECT_HEIGHT )
		use_wide_aspect = 1;
	if( tracks->recordable_audio_tracks() == BD_WIDE_CHANNELS )
		use_wide_audio = 1;
	return 0;
}

