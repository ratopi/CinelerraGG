#ifndef __BDCREATE_H__
#define __BDCREATE_H__

#include "arraylist.h"
#include "batchrender.h"
#include "bcwindowbase.h"
#include "bcbutton.h"
#include "bcdialog.h"
#include "bcmenuitem.h"
#include "bctextbox.h"
#include "mwindow.h"

#include "bdcreate.inc"




class CreateBD_MenuItem : public BC_MenuItem
{
public:
	CreateBD_MenuItem(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};


class CreateBD_Thread : public BC_DialogThread
{
	static const int64_t BD_SIZE;
	static const int BD_STREAMS, BD_WIDTH, BD_HEIGHT;
	static const double BD_ASPECT_WIDTH, BD_ASPECT_HEIGHT;
	static const double BD_WIDE_ASPECT_WIDTH, BD_WIDE_ASPECT_HEIGHT;
	static const int BD_MAX_BITRATE, BD_CHANNELS, BD_WIDE_CHANNELS;
	static const double BD_FRAMERATE, BD_SAMPLERATE, BD_KAUDIO_RATE;
public:
	CreateBD_Thread(MWindow *mwindow);
	~CreateBD_Thread();
	void handle_close_event(int result);
	BC_Window* new_gui();
	int option_presets();
	int create_bd_jobs(ArrayList<BatchRenderJob*> *jobs,
		const char *tmp_path, const char *asset_title);
	int insert_video_plugin(const char *title, KeyFrame *default_keyframe);
	int resize_tracks();

	MWindow *mwindow;
	CreateBD_GUI *gui;
	char asset_title[BCTEXTLEN];
	char tmp_path[BCTEXTLEN];
	int use_deinterlace, use_inverse_telecine;
	int use_scale, use_resize_tracks;
	int use_wide_audio, use_wide_aspect;
	int use_histogram, use_label_chapters;
};

class CreateBD_OK : public BC_OKButton
{
public:
	CreateBD_OK(CreateBD_GUI *gui, int x, int y);
	~CreateBD_OK();
	int button_press_event();
	int keypress_event();

	CreateBD_GUI *gui;
};

class CreateBD_Cancel : public BC_CancelButton
{
public:
	CreateBD_Cancel(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Cancel();
	int button_press_event();

	CreateBD_GUI *gui;
};


class CreateBD_DiskSpace : public BC_Title
{
public:
	CreateBD_DiskSpace(CreateBD_GUI *gui, int x, int y);
	~CreateBD_DiskSpace();
	int64_t tmp_path_space();
	void update();

	CreateBD_GUI *gui;
};

class CreateBD_TmpPath : public BC_TextBox
{
public:
	CreateBD_TmpPath(CreateBD_GUI *gui, int x, int y, int w);
	~CreateBD_TmpPath();
	int handle_event();

	CreateBD_GUI *gui;
};


class CreateBD_AssetTitle : public BC_TextBox
{
public:
	CreateBD_AssetTitle(CreateBD_GUI *gui, int x, int y, int w);
	~CreateBD_AssetTitle();

	CreateBD_GUI *gui;
};

class CreateBD_Deinterlace : public BC_CheckBox
{
public:
	CreateBD_Deinterlace(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Deinterlace();
	int handle_event();

	CreateBD_GUI *gui;
};

class CreateBD_InverseTelecine : public BC_CheckBox
{
public:
	CreateBD_InverseTelecine(CreateBD_GUI *gui, int x, int y);
	~CreateBD_InverseTelecine();
	int handle_event();

	CreateBD_GUI *gui;
};

class CreateBD_Scale : public BC_CheckBox
{
public:
	CreateBD_Scale(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Scale();

	CreateBD_GUI *gui;
};

class CreateBD_ResizeTracks : public BC_CheckBox
{
public:
	CreateBD_ResizeTracks(CreateBD_GUI *gui, int x, int y);
	~CreateBD_ResizeTracks();

	CreateBD_GUI *gui;
};

class CreateBD_Histogram : public BC_CheckBox
{
public:
	CreateBD_Histogram(CreateBD_GUI *gui, int x, int y);
	~CreateBD_Histogram();

	CreateBD_GUI *gui;
};

class CreateBD_LabelChapters : public BC_CheckBox
{
public:
	CreateBD_LabelChapters(CreateBD_GUI *gui, int x, int y);
	~CreateBD_LabelChapters();

	CreateBD_GUI *gui;
};

class CreateBD_WideAudio : public BC_CheckBox
{
public:
	CreateBD_WideAudio(CreateBD_GUI *gui, int x, int y);
	~CreateBD_WideAudio();

	CreateBD_GUI *gui;
};

class CreateBD_WideAspect : public BC_CheckBox
{
public:
	CreateBD_WideAspect(CreateBD_GUI *gui, int x, int y);
	~CreateBD_WideAspect();

	CreateBD_GUI *gui;
};

class CreateBD_GUI : public BC_Window
{
public:
	CreateBD_GUI(CreateBD_Thread *thread,
		int x, int y, int w, int h);
	~CreateBD_GUI();

	void create_objects();
	int resize_event(int w, int h);
	int translation_event();
	int close_event();

	int64_t needed_disk_space;
	CreateBD_Thread *thread;
	int at_x, at_y;
	CreateBD_AssetTitle *asset_title;
	int tmp_x, tmp_y;
	CreateBD_TmpPath *tmp_path;
	CreateBD_DiskSpace *disk_space;
	CreateBD_Deinterlace *need_deinterlace;
	CreateBD_InverseTelecine *need_inverse_telecine;
	CreateBD_Scale *need_scale;
	CreateBD_ResizeTracks *need_resize_tracks;
	CreateBD_Histogram *need_histogram;
	CreateBD_WideAudio *need_wide_audio;
	CreateBD_WideAspect *need_wide_aspect;
	CreateBD_LabelChapters *need_label_chapters;
	int ok_x, ok_y, ok_w, ok_h;
	CreateBD_OK *ok;
	int cancel_x, cancel_y, cancel_w, cancel_h;
	CreateBD_Cancel *cancel;
};

#endif
