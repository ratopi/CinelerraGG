
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
// work around for __STDC_CONSTANT_MACROS
#include <lzma.h>

#include "asset.h"
#include "bcwindowbase.h"
#include "bitspopup.h"
#include "ffmpeg.h"
#include "filebase.h"
#include "file.h"
#include "fileffmpeg.h"
#include "filesystem.h"
#include "mutex.h"
#include "videodevice.inc"

FileFFMPEG::FileFFMPEG(Asset *asset, File *file)
  : FileBase(asset, file)
{
	ff = 0;
	if(asset->format == FILE_UNKNOWN)
		asset->format = FILE_FFMPEG;
}

FileFFMPEG::~FileFFMPEG()
{
	delete ff;
}


FFMpegConfigNum::FFMpegConfigNum(BC_Window *window,
		int x, int y, char *title_text, int *output)
 : BC_TumbleTextBox(window, (int64_t)*output,
	(int64_t)-1, (int64_t)25000000, 100, y, 100)
{
	this->window = window;
	this->x = x;  this->y = y;
	this->title_text = title_text;
	this->output = output;
}

FFMpegConfigNum::~FFMpegConfigNum()
{
}

void FFMpegConfigNum::create_objects()
{
	window->add_subwindow(title = new BC_Title(x, y, title_text));
	BC_TumbleTextBox::create_objects();
}

int FFMpegConfigNum::handle_event()
{
	*output = atol(get_text());
	return 1;
}

FFMpegAudioNum::FFMpegAudioNum(BC_Window *window,
		int x, int y, char *title_text, int *output)
 : FFMpegConfigNum(window, x, y, title_text, output)
{
}

int FFMpegAudioBitrate::handle_event()
{
	int ret = FFMpegAudioNum::handle_event();
	return ret;
}

FFMpegVideoNum::FFMpegVideoNum(BC_Window *window,
		int x, int y, char *title_text, int *output)
 : FFMpegConfigNum(window, x, y, title_text, output)
{
}

int FFMpegVideoBitrate::handle_event()
{
	int ret = FFMpegVideoNum::handle_event();
	Asset *asset = window()->asset;
	if( asset->ff_video_bitrate )
		window()->quality->disable();
	else
		window()->quality->enable();
	return ret;
}

int FFMpegVideoQuality::handle_event()
{
	int ret = FFMpegVideoNum::handle_event();
	Asset *asset = window()->asset;
	if( asset->ff_video_quality )
		window()->bitrate->disable();
	else
		window()->bitrate->enable();
	return ret;
}

void FileFFMPEG::get_parameters(BC_WindowBase *parent_window,
		Asset *asset, BC_WindowBase *&format_window,
		int audio_options, int video_options)
{
	if(audio_options) {
		FFMPEGConfigAudio *window = new FFMPEGConfigAudio(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
	else if(video_options) {
		FFMPEGConfigVideo *window = new FFMPEGConfigVideo(parent_window, asset);
		format_window = window;
		window->create_objects();
		window->run_window();
		delete window;
	}
}

int FileFFMPEG::check_sig(Asset *asset)
{
	char *ptr = strstr(asset->path, ".pcm");
	if( ptr ) return 0;
	ptr = strstr(asset->path, ".raw");
	if( ptr ) return 0;

	FFMPEG ffmpeg(0);
	int ret = !ffmpeg.init_decoder(asset->path) &&
		!ffmpeg.open_decoder() ? 1 : 0;
	return ret;
}

void FileFFMPEG::get_info(char *path, char *text)
{
	char *cp = text;
	FFMPEG ffmpeg(0);
	cp += sprintf(cp, "file path: %s\n", path);
	struct stat st;
	int ret = 0;
	if( stat(path, &st) < 0 ) {
		cp += sprintf(cp, " err: %s\n", strerror(errno));
		ret = 1;
	}
	else {
		cp += sprintf(cp, "  %jd bytes\n", st.st_size);
	}
	if( !ret ) ret = ffmpeg.init_decoder(path);
	if( !ret ) ret = ffmpeg.open_decoder();
	if( !ret ) {
		cp += sprintf(cp, "info:\n");
		ffmpeg.info(cp, BCTEXTLEN-(cp-text));
	}
	else
		sprintf(cp, "== open failed\n");
}

int FileFFMPEG::get_video_info(int track, int &pid, double &framerate,
		int &width, int &height, char *title)
{
        if( !ff ) return -1;
        pid = ff->ff_video_pid(track);
        framerate = ff->ff_frame_rate(track);
        width = ff->ff_video_width(track);
        height = ff->ff_video_height(track);
        if( title ) *title = 0;
	return 0;
}

int FileFFMPEG::get_audio_for_video(int vstream, int astream, int64_t &channel_mask)
{
	if( !ff ) return 1;
	return ff->ff_audio_for_video(vstream, astream, channel_mask);
}

int FileFFMPEG::select_video_stream(Asset *asset, int vstream)
{
	if( !ff || !asset->video_data ) return 1;
	asset->width = ff->ff_video_width(vstream);
	asset->height = ff->ff_video_height(vstream);
	asset->video_length = ff->ff_video_frames(vstream);
	asset->frame_rate = ff->ff_frame_rate(vstream);
	return 0;
}

int FileFFMPEG::select_audio_stream(Asset *asset, int astream)
{
	if( !ff || !asset->audio_data ) return 1;
	asset->channels = ff->ff_audio_channels(astream);
	asset->sample_rate = ff->ff_sample_rate(astream);
       	asset->audio_length = ff->ff_audio_samples(astream);
	return 0;
}

int FileFFMPEG::open_file(int rd, int wr)
{
	int result = 0;
	if( ff ) return 1;
	ff = new FFMPEG(this);

	if( rd ) {
		result = ff->init_decoder(asset->path);
		if( !result ) result = ff->open_decoder();
		if( !result ) {
			int audio_channels = ff->ff_total_audio_channels();
			if( audio_channels > 0 ) {
				asset->audio_data = 1;
				asset->channels = audio_channels;
				asset->sample_rate = ff->ff_sample_rate(0);
		        	asset->audio_length = ff->ff_audio_samples(0);
			}
			int video_layers = ff->ff_total_video_layers();
			if( video_layers > 0 ) {
				asset->video_data = 1;
				if( !asset->layers ) asset->layers = video_layers;
				asset->actual_width = ff->ff_video_width(0);
				asset->actual_height = ff->ff_video_height(0);
				if( !asset->width ) asset->width = asset->actual_width;
				if( !asset->height ) asset->height = asset->actual_height;
				if( !asset->video_length ) asset->video_length = ff->ff_video_frames(0);
				if( !asset->frame_rate ) asset->frame_rate = ff->ff_frame_rate(0);
			}
		}
	}
	else if( wr ) {
		result = ff->init_encoder(asset->path);
		// must be in this order or dvdauthor will fail
		if( !result && asset->video_data )
			result = ff->open_encoder("video", asset->vcodec);
		if( !result && asset->audio_data )
			result = ff->open_encoder("audio", asset->acodec);
	}
	return result;
}

int FileFFMPEG::close_file()
{
	delete ff;
	ff = 0;
	return 0;
}


int FileFFMPEG::set_video_position(int64_t pos)
{
        if( !ff || pos < 0 || pos >= asset->video_length )
		return 1;
	return 0;
}


int FileFFMPEG::set_audio_position(int64_t pos)
{
        if( !ff || pos < 0 || pos >= asset->audio_length )
		return 1;
	return 0;
}


int FileFFMPEG::write_samples(double **buffer, int64_t len)
{
        if( !ff || len < 0 ) return -1;
	int stream = 0;
	int ret = ff->encode(stream, buffer, len);
	return ret;
}

int FileFFMPEG::write_frames(VFrame ***frames, int len)
{
        if( !ff ) return -1;
	int ret = 0, layer = 0;
	for(int i = 0; i < 1; i++) {
		for(int j = 0; j < len && !ret; j++) {
			VFrame *frame = frames[i][j];
			ret = ff->encode(layer, frame);
		}
	}
	return ret;
}


int FileFFMPEG::read_samples(double *buffer, int64_t len)
{
        if( !ff || len < 0 ) return -1;
	int ch = file->current_channel;
	int64_t pos = file->current_sample;
	ff->decode(ch, pos, buffer, len);
        return 0;
}

int FileFFMPEG::read_frame(VFrame *frame)
{
        if( !ff ) return -1;
	int layer = file->current_layer;
	int64_t pos = file->current_frame;
	ff->decode(layer, pos, frame);
	return 0;
}


int64_t FileFFMPEG::get_memory_usage()
{
	return 0;
}

int FileFFMPEG::colormodel_supported(int colormodel)
{
	return colormodel;
}

int FileFFMPEG::get_best_colormodel(Asset *asset, int driver)
{
	switch(driver) {
	case PLAYBACK_X11:	return BC_RGB888;
	case PLAYBACK_X11_GL:	return BC_YUV888;
        }
	return BC_YUV420P;
}

//======
extern void get_exe_path(char *result); // from main.C

FFMPEGConfigAudio::FFMPEGConfigAudio(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Audio Preset",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	420, 420)
{
	this->parent_window = parent_window;
	this->asset = asset;
	preset_popup = 0;

	bitrate = 0;
	audio_options = 0;
}

FFMPEGConfigAudio::~FFMPEGConfigAudio()
{
	lock_window("FFMPEGConfigAudio::~FFMPEGConfigAudio");
	if(preset_popup) delete preset_popup;
	presets.remove_all_objects();
	unlock_window();
}

void FFMPEGConfigAudio::create_objects()
{
	int x = 10, y = 10;
	lock_window("FFMPEGConfigAudio::create_objects");

        FileSystem fs;
	char option_path[BCTEXTLEN];
	FFMPEG::set_option_path(option_path, "audio");
        fs.update(option_path);
        int total_files = fs.total_files();
        for(int i = 0; i < total_files; i++) {
                const char *name = fs.get_entry(i)->get_name();
		if( asset->fformat[0] != 0 ) {
			const char *ext = strrchr(name,'.');
			if( !ext ) continue;
			if( strcmp(asset->fformat, ++ext) ) continue;
		}
                presets.append(new BC_ListBoxItem(name));
        }

	if( asset->acodec[0] ) {
		int k = presets.size();
		while( --k >= 0 && strcmp(asset->acodec, presets[k]->get_text()) );
		if( k < 0 ) asset->acodec[0] = 0;
	}

	if( !asset->acodec[0] && presets.size() > 0 )
		strcpy(asset->acodec, presets[0]->get_text());

	add_tool(new BC_Title(x, y, _("Preset:")));
	y += 25;
	preset_popup = new FFMPEGConfigAudioPopup(this, x, y);
	preset_popup->create_objects();

	y += 50;
	bitrate = new FFMpegAudioBitrate(this, x, y, _("Bitrate:"), &asset->ff_audio_bitrate);
	bitrate->create_objects();
	bitrate->set_increment(1000);

	y += bitrate->get_h() + 10;
	add_subwindow(new BC_Title(x, y, _("Audio Options:")));
	y += 25;
	if( !asset->ff_audio_options[0] && asset->acodec[0] ) {
		FFMPEG::set_option_path(option_path, "audio/%s", asset->acodec);
		FFMPEG::load_options(option_path, asset->ff_audio_options,
			 sizeof(asset->ff_audio_options));
	}
	add_subwindow(audio_options = new FFAudioOptions(this, x, y, get_w()-x-20, 10,
		 sizeof(asset->ff_audio_options)-1, asset->ff_audio_options));
	add_subwindow(new BC_OKButton(this));
	show_window(1);
	bitrate->handle_event();
	unlock_window();
}

int FFMPEGConfigAudio::close_event()
{
	set_done(0);
	return 1;
}


FFAudioOptions::FFAudioOptions(FFMPEGConfigAudio *audio_popup,
	int x, int y, int w, int rows, int size, char *text)
 : BC_TextBox(x, y, w, rows, size, text)
{
	this->audio_popup = audio_popup;
}

int FFAudioOptions::handle_event()
{
	strcpy(audio_popup->asset->ff_audio_options, get_text());
	return 1;
}


FFMPEGConfigAudioPopup::FFMPEGConfigAudioPopup(FFMPEGConfigAudio *popup, int x, int y)
 : BC_PopupTextBox(popup, &popup->presets, popup->asset->acodec, x, y, 300, 300)
{
	this->popup = popup;
}

int FFMPEGConfigAudioPopup::handle_event()
{
	strcpy(popup->asset->acodec, get_text());
	Asset *asset = popup->asset;
	char option_path[BCTEXTLEN];
	FFMPEG::set_option_path(option_path, "audio/%s", asset->acodec);
	FFMPEG::load_options(option_path, asset->ff_audio_options,
			 sizeof(asset->ff_audio_options));
	popup->audio_options->update(asset->ff_audio_options);
	return 1;
}


FFMPEGConfigAudioToggle::FFMPEGConfigAudioToggle(FFMPEGConfigAudio *popup,
	char *title_text, int x, int y, int *output)
 : BC_CheckBox(x, y, *output, title_text)
{
	this->popup = popup;
	this->output = output;
}
int FFMPEGConfigAudioToggle::handle_event()
{
	*output = get_value();
	return 1;
}


//======

FFMPEGConfigVideo::FFMPEGConfigVideo(BC_WindowBase *parent_window, Asset *asset)
 : BC_Window(PROGRAM_NAME ": Video Preset",
 	parent_window->get_abs_cursor_x(1),
 	parent_window->get_abs_cursor_y(1),
	420, 420)
{
	this->parent_window = parent_window;
	this->asset = asset;
	preset_popup = 0;

	bitrate = 0;
	quality = 0;
	video_options = 0;
}

FFMPEGConfigVideo::~FFMPEGConfigVideo()
{
	lock_window("FFMPEGConfigVideo::~FFMPEGConfigVideo");
	if(preset_popup) delete preset_popup;
	presets.remove_all_objects();
	unlock_window();
}

void FFMPEGConfigVideo::create_objects()
{
	int x = 10, y = 10;
	lock_window("FFMPEGConfigVideo::create_objects");

	add_subwindow(new BC_Title(x, y, _("Compression:")));
	y += 25;

        FileSystem fs;
	char option_path[BCTEXTLEN];
	FFMPEG::set_option_path(option_path, "video");
        fs.update(option_path);
        int total_files = fs.total_files();
        for(int i = 0; i < total_files; i++) {
                const char *name = fs.get_entry(i)->get_name();
		if( asset->fformat[0] != 0 ) {
			const char *ext = strrchr(name,'.');
			if( !ext ) continue;
			if( strcmp(asset->fformat, ++ext) ) continue;
		}
                presets.append(new BC_ListBoxItem(name));
        }

	if( asset->vcodec[0] ) {
		int k = presets.size();
		while( --k >= 0 && strcmp(asset->vcodec, presets[k]->get_text()) );
		if( k < 0 ) asset->vcodec[0] = 0;
	}

	if( !asset->vcodec[0] && presets.size() > 0 )
		strcpy(asset->vcodec, presets[0]->get_text());

	preset_popup = new FFMPEGConfigVideoPopup(this, x, y);
	preset_popup->create_objects();

	if( asset->ff_video_bitrate && asset->ff_video_quality ) {
		asset->ff_video_bitrate = 0;
		asset->ff_video_quality = 0;
	}

	y += 50;
	bitrate = new FFMpegVideoBitrate(this, x, y, _("Bitrate:"), &asset->ff_video_bitrate);
	bitrate->create_objects();
	bitrate->set_increment(100000);
	y += bitrate->get_h() + 5;
	quality = new FFMpegVideoQuality(this, x, y, _("Quality:"), &asset->ff_video_quality);
	quality->create_objects();
	quality->set_increment(1);
	quality->set_boundaries((int64_t)0, (int64_t)31);

	y += quality->get_h() + 10;
	add_subwindow(new BC_Title(x, y, _("Video Options:")));
	y += 25;
	if( !asset->ff_video_options[0] && asset->vcodec[0] ) {
		FFMPEG::set_option_path(option_path, "video/%s", asset->vcodec);
		FFMPEG::load_options(option_path, asset->ff_video_options,
			 sizeof(asset->ff_video_options));
	}
	add_subwindow(video_options = new FFVideoOptions(this, x, y, get_w()-x-20, 10,
		 sizeof(asset->ff_video_options)-1, asset->ff_video_options));

	add_subwindow(new BC_OKButton(this));
	show_window(1);
	if( asset->ff_video_bitrate )
		quality->disable();
	if( asset->ff_video_quality )
		bitrate->disable();
	unlock_window();
}

int FFMPEGConfigVideo::close_event()
{
	set_done(0);
	return 1;
}


FFVideoOptions::FFVideoOptions(FFMPEGConfigVideo *video_popup,
	int x, int y, int w, int rows, int size, char *text)
 : BC_TextBox(x, y, w, rows, size, text)
{
	this->video_popup = video_popup;
}

int FFVideoOptions::handle_event()
{
	strcpy(video_popup->asset->ff_video_options, get_text());
	return 1;
}


FFMPEGConfigVideoPopup::FFMPEGConfigVideoPopup(FFMPEGConfigVideo *popup, int x, int y)
 : BC_PopupTextBox(popup, &popup->presets, popup->asset->vcodec, x, y, 300, 300)
{
	this->popup = popup;
}

int FFMPEGConfigVideoPopup::handle_event()
{
	strcpy(popup->asset->vcodec, get_text());
	Asset *asset = popup->asset;
	char option_path[BCTEXTLEN];
	FFMPEG::set_option_path(option_path, "video/%s", asset->vcodec);
	FFMPEG::load_options(option_path, asset->ff_video_options,
			 sizeof(asset->ff_video_options));
	popup->video_options->update(asset->ff_video_options);
	return 1;
}


FFMPEGConfigVideoToggle::FFMPEGConfigVideoToggle(FFMPEGConfigVideo *popup,
	char *title_text, int x, int y, int *output)
 : BC_CheckBox(x, y, *output, title_text)
{
	this->popup = popup;
	this->output = output;
}
int FFMPEGConfigVideoToggle::handle_event()
{
	*output = get_value();
	return 1;
}


