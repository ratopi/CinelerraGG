#ifndef __FILEFFMPEG_H__
#define __FILEFFMPEG_H__

#include "asset.inc" 
#include "bcwindowbase.inc"
#include "bitspopup.inc" 
#include "filebase.h"
#include "fileffmpeg.inc"
#include "mwindow.inc"
#include "mutex.h"
#include "vframe.inc"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

class FileFFMPEG;
class FFMpegConfigNum;
class FFMpegAudioNum;
class FFMpegVideoNum;
class FFOkButton;
class FFOptions;
class FFMPEGConfigAudio;
class FFAudioOptions;
class FFMPEGConfigAudioPopup;
class FFMPEGConfigAudioToggle;
class FFMPEGConfigVideo;
class FFVideoOptions;
class FFMPEGConfigVideoPopup;
class FFMPEGConfigVideoToggle;

class FileFFMPEG : public FileBase
{
public:
	FFMPEG *ff;

        FileFFMPEG(Asset *asset, File *file);
        ~FileFFMPEG();
	static void ff_lock(const char *cp=0);
	static void ff_unlock();

	static void get_parameters(BC_WindowBase *parent_window,Asset *asset,
	   BC_WindowBase *&format_window,int audio_options,int video_options);
	static int check_sig(Asset *asset);
	int get_video_info(int track, int &pid, double &framerate,
		int &width, int &height, char *title=0);
	int get_audio_for_video(int vstream, int astream, int64_t &channel_mask);
	static void get_info(char *path,char *text);
	int open_file(int rd,int wr);
	int close_file(void);
	int set_video_position(int64_t pos);
	int set_audio_position(int64_t pos);
	int write_samples(double **buffer,int64_t len);
	int write_frames(VFrame ***frames,int len);
	int read_samples(double *buffer,int64_t len);
	int read_frame(VFrame *frame);
	int64_t get_memory_usage(void);
	int colormodel_supported(int colormodel);
	int get_best_colormodel(Asset *asset,int driver);
	int select_video_stream(Asset *asset, int vstream);
	int select_audio_stream(Asset *asset, int astream);
};

class FFMpegConfigNum : public BC_TumbleTextBox
{
public:
        FFMpegConfigNum(BC_Window *window, int x, int y,
                char *title_text, int *output);
        ~FFMpegConfigNum();

        void create_objects();
        int handle_event();
        int *output;
        BC_Window *window;
        BC_Title *title;
        char *title_text;
        int x, y;
};

class FFMpegAudioNum : public FFMpegConfigNum
{
public:
        FFMpegAudioNum(BC_Window *window, int x, int y, char *title_text, int *output);
        ~FFMpegAudioNum() {}

        FFMPEGConfigAudio *window() { return (FFMPEGConfigAudio *)FFMpegConfigNum::window; }
};

class FFMpegAudioBitrate : public FFMpegAudioNum
{
public:
        FFMpegAudioBitrate(BC_Window *window, int x, int y, char *title_text, int *output)
          : FFMpegAudioNum(window, x, y, title_text, output) {}
	int handle_event();
};

class FFMpegVideoNum : public FFMpegConfigNum
{
public:
        FFMpegVideoNum(BC_Window *window, int x, int y, char *title_text, int *output);
        ~FFMpegVideoNum() {}

        FFMPEGConfigVideo *window() { return (FFMPEGConfigVideo *)FFMpegConfigNum::window; }
};

class FFMpegVideoBitrate : public FFMpegVideoNum
{
public:
        FFMpegVideoBitrate(BC_Window *window, int x, int y, char *title_text, int *output)
          : FFMpegVideoNum(window, x, y, title_text, output) {}
	int handle_event();
};

class FFMpegVideoQuality : public FFMpegVideoNum
{
public:
        FFMpegVideoQuality(BC_Window *window, int x, int y, char *title_text, int *output)
          : FFMpegVideoNum(window, x, y, title_text, output) {}
	int handle_event();
};

class FFMPEGConfigAudio : public BC_Window
{
public:
	FFMPEGConfigAudio(BC_WindowBase *parent_window, Asset *asset);
	~FFMPEGConfigAudio();

	void create_objects();
	int close_event();

	ArrayList<BC_ListBoxItem*> presets;
	FFMPEGConfigAudioPopup *preset_popup;
	FFMpegAudioBitrate *bitrate;
	FFAudioOptions *audio_options;
	BC_WindowBase *parent_window;
	Asset *asset;
};

class FFAudioOptions : public BC_TextBox
{
public:
	FFAudioOptions(FFMPEGConfigAudio *audio_popup,
		int x, int y, int w, int rows, int size, char *text);
	int handle_event();

	FFMPEGConfigAudio *audio_popup;
};


class FFMPEGConfigAudioPopup : public BC_PopupTextBox
{
public:
	FFMPEGConfigAudioPopup(FFMPEGConfigAudio *popup, int x, int y);
	int handle_event();
	FFMPEGConfigAudio *popup;
};


class FFMPEGConfigAudioToggle : public BC_CheckBox
{
public:
	FFMPEGConfigAudioToggle(FFMPEGConfigAudio *popup,
		char *title_text, int x, int y, int *output);
	int handle_event();
	int *output;
	FFMPEGConfigAudio *popup;
};

class FFMPEGConfigVideo : public BC_Window
{
public:
	FFMPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset);
	~FFMPEGConfigVideo();

	void create_objects();
	int close_event();

	ArrayList<BC_ListBoxItem*> presets;
	FFMPEGConfigVideoPopup *preset_popup;
	BC_WindowBase *parent_window;
	FFMpegVideoBitrate *bitrate;
	FFMpegVideoQuality *quality;
	FFVideoOptions *video_options;
	Asset *asset;
};

class FFVideoOptions : public BC_TextBox
{
public:
	FFVideoOptions(FFMPEGConfigVideo *video_popup,
		int x, int y, int w, int rows, int size, char *text);
	int handle_event();

	FFMPEGConfigVideo *video_popup;
};

class FFMPEGConfigVideoPopup : public BC_PopupTextBox
{
public:
	FFMPEGConfigVideoPopup(FFMPEGConfigVideo *popup, int x, int y);
	int handle_event();
	FFMPEGConfigVideo *popup;
};

class FFMPEGConfigVideoToggle : public BC_CheckBox
{
public:
	FFMPEGConfigVideoToggle(FFMPEGConfigVideo *popup,
		char *title_text, int x, int y, int *output);
	int handle_event();
	int *output;
	FFMPEGConfigVideo *popup;
};

#endif
