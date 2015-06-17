#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include "funcprotos.h"
#include "quicktime.h"


static int total_vcodecs = 0;
static int total_acodecs = 0;
static quicktime_codectable_t *vcodecs = NULL;
static quicktime_codectable_t *acodecs = NULL;




static int register_vcodec(void (*init_vcodec)(quicktime_video_map_t *),
		const char *fourcc, int vid_id)
{
	int idx = total_vcodecs++;
	const uint8_t *bp = (const uint8_t *)fourcc;
	vcodecs = (quicktime_codectable_t *)realloc(vcodecs,
		total_vcodecs * sizeof(quicktime_codectable_t));
	quicktime_codectable_t *codec = vcodecs + idx;
	codec->fourcc = (bp[3]<<24) | (bp[2]<<16) | (bp[1]<<8) | (bp[0]<<0);
	codec->id = vid_id;
	codec->init_vcodec = init_vcodec;
	return idx;
}

static int register_acodec(void (*init_acodec)(quicktime_audio_map_t *),
		const char *fourcc, int wav_id)
{
	int idx = total_acodecs++;
	const uint8_t *bp = (const uint8_t *)fourcc;
	acodecs = (quicktime_codectable_t *)realloc(acodecs,
		total_acodecs * sizeof(quicktime_codectable_t));
	quicktime_codectable_t *codec = acodecs + idx;
	codec->fourcc = (bp[3]<<24) | (bp[2]<<16) | (bp[1]<<8) | (bp[0]<<0);
	codec->id = wav_id;
	codec->init_acodec = init_acodec;
	return idx;
}




#include "ima4.h"
#include "mp4a.h"
#include "qdm2.h"
#include "qtvorbis.h"
#include "qtmp3.h"
#include "rawaudio.h"
#include "twos.h"
#include "ulaw.h"
#include "wma.h"
#include "wmx2.h"

static void register_acodecs()
{
	register_acodec(quicktime_init_codec_twos, QUICKTIME_TWOS, 0x01);
	register_acodec(quicktime_init_codec_sowt, QUICKTIME_SOWT, 0x01);
	register_acodec(quicktime_init_codec_rawaudio, QUICKTIME_RAW, 0x01);
	register_acodec(quicktime_init_codec_ima4, QUICKTIME_IMA4, 0x11);
	register_acodec(quicktime_init_codec_mp4a, QUICKTIME_MP4A, 0);
	register_acodec(quicktime_init_codec_qdm2, QUICKTIME_QDM2, 0);
	register_acodec(quicktime_init_codec_ulaw, QUICKTIME_ULAW, 0x07);

	register_acodec(quicktime_init_codec_vorbis, QUICKTIME_VORBIS, 0x01);
	register_acodec(quicktime_init_codec_mp3, QUICKTIME_MP3, 0x55);
	register_acodec(quicktime_init_codec_wmx2, QUICKTIME_WMX2, 0x11);
	register_acodec(quicktime_init_codec_wmav1, QUICKTIME_WMA, 0x160);
	register_acodec(quicktime_init_codec_wmav2, QUICKTIME_WMA, 0x161);
}





#include "qth264.h"
#include "raw.h"
#include "qtdv.h"
#include "jpeg.h"
#include "mpeg4.h"
#include "qtpng.h"
#include "rle.h"
#include "v308.h"
#include "v408.h"
#include "v410.h"
#include "yuv2.h"
#include "yuv4.h"
#include "yv12.h"

static void register_vcodecs()
{

	register_vcodec(quicktime_init_codec_raw, QUICKTIME_RAW, 0);

	register_vcodec(quicktime_init_codec_h264, QUICKTIME_H264, 1);
	register_vcodec(quicktime_init_codec_hv64, QUICKTIME_HV64, 2);
	register_vcodec(quicktime_init_codec_divx, QUICKTIME_DIVX, 0);
	register_vcodec(quicktime_init_codec_hv60, QUICKTIME_HV60, 0);
	register_vcodec(quicktime_init_codec_div5, QUICKTIME_DX50, 0);
	register_vcodec(quicktime_init_codec_div3, QUICKTIME_DIV3, 0);
	register_vcodec(quicktime_init_codec_div3v2, QUICKTIME_MP42, 0);
	register_vcodec(quicktime_init_codec_div3lower, QUICKTIME_DIV3_LOWER, 0);
	register_vcodec(quicktime_init_codec_mp4v, QUICKTIME_MP4V, 0);
	register_vcodec(quicktime_init_codec_xvid, QUICKTIME_XVID, 0);
	register_vcodec(quicktime_init_codec_dnxhd, QUICKTIME_DNXHD, 0);
	register_vcodec(quicktime_init_codec_svq1, QUICKTIME_SVQ1, 0);
	register_vcodec(quicktime_init_codec_svq3, QUICKTIME_SVQ3, 0);
	register_vcodec(quicktime_init_codec_h263, QUICKTIME_H263, 0);
	register_vcodec(quicktime_init_codec_dv, QUICKTIME_DV, 0);
	register_vcodec(quicktime_init_codec_dvsd, QUICKTIME_DVSD, 0);
	register_vcodec(quicktime_init_codec_dvcp, QUICKTIME_DVCP, 0);

	register_vcodec(quicktime_init_codec_jpeg, QUICKTIME_JPEG, 0);
	register_vcodec(quicktime_init_codec_mjpa, QUICKTIME_MJPA, 0);
	register_vcodec(quicktime_init_codec_mjpg, QUICKTIME_MJPG, 0);
	register_vcodec(quicktime_init_codec_png, QUICKTIME_PNG, 0);
	register_vcodec(quicktime_init_codec_rle, QUICKTIME_RLE, 0);

	register_vcodec(quicktime_init_codec_yuv2, QUICKTIME_YUV2, 0);
	register_vcodec(quicktime_init_codec_2vuy, QUICKTIME_2VUY, 0);
	register_vcodec(quicktime_init_codec_yuv4, QUICKTIME_YUV4, 0);
	register_vcodec(quicktime_init_codec_yv12, QUICKTIME_YUV420, 0);
	register_vcodec(quicktime_init_codec_v410, QUICKTIME_YUV444_10bit, 0);
	register_vcodec(quicktime_init_codec_v308, QUICKTIME_YUV444, 0);
	register_vcodec(quicktime_init_codec_v408, QUICKTIME_YUVA4444, 0);
}




int quicktime_find_vcodec(quicktime_video_map_t *vtrack)
{
	int i;
	quicktime_codec_t *codec_base = 0;
	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	const uint8_t *bp = (const uint8_t *)compressor;
	uint32_t fourcc = (bp[3]<<24) | (bp[2]<<16) | (bp[1]<<8) | (bp[0]<<0);
	if(!total_vcodecs) register_vcodecs();
	for(i = 0; i<total_vcodecs; ++i ) {
		if( fourcc == vcodecs[i].fourcc ) break;
	}
	if( i >= total_vcodecs ) return -1;
	if( !(codec_base = quicktime_new_codec()) ) return -1;
	vtrack->codec = codec_base;
	vcodecs[i].init_vcodec(vtrack);
	return 0;
}

int quicktime_find_acodec(quicktime_audio_map_t *atrack)
{
	int i;
	char *compressor = atrack->track->mdia.minf.stbl.stsd.table[0].format;
	int compression_id = atrack->track->mdia.minf.stbl.stsd.table[0].compression_id;
	const uint8_t *bp = (const uint8_t *)compressor;
	uint32_t fourcc = (bp[3]<<24) | (bp[2]<<16) | (bp[1]<<8) | (bp[0]<<0);
	if(!total_acodecs) register_acodecs();
	quicktime_codec_t *codec_base = 0;

	for(i = 0; i<total_acodecs; ++i ) {
		if( fourcc == acodecs[i].fourcc ) break;
// For reading AVI, sometimes the fourcc is 0 and the compression_id is used instead.
// Sometimes the compression_id is the fourcc.
		if( acodecs[i].id != compression_id ) continue;
		if( !compressor[0] || fourcc == acodecs[i].id ) break;
	}
	if( i >= total_acodecs ) return -1;
	if( !(codec_base = quicktime_new_codec()) ) return -1;
	atrack->codec = codec_base;
	acodecs[i].init_acodec(atrack);
// For writing and reading Quicktime
	quicktime_copy_char32(compressor, codec_base->fourcc);
	return 0;
}


char* quicktime_acodec_title(char *fourcc)
{
	int i;
	char *result = 0;
	quicktime_audio_map_t *atrack = (quicktime_audio_map_t*)
		calloc(1, sizeof(quicktime_audio_map_t));
	if(!total_acodecs) register_acodecs();
	quicktime_codec_t *codec_base = 0;

	int done = 0;
	for(i = 0; i < total_acodecs && !done; i++) {
		if( !(codec_base = quicktime_new_codec()) ) return 0;
		atrack->codec = codec_base;
		acodecs[i].init_acodec(atrack);
		if( quicktime_match_32(fourcc, codec_base->fourcc) ) {
			result = codec_base->title;
			done = 1;
		}
		codec_base->delete_acodec(atrack);
		quicktime_del_codec(codec_base);
	}

	free(atrack);
	return result ? result : fourcc;
}

char* quicktime_vcodec_title(char *fourcc)
{
	int i;
	char *result = 0;

	quicktime_video_map_t *vtrack =
		(quicktime_video_map_t*)calloc(1, sizeof(quicktime_video_map_t));
	if(!total_vcodecs) register_vcodecs();
	quicktime_codec_t *codec_base = 0;

	int done = 0;
	for(i = 0; i < total_vcodecs && !done; i++) {
		if( !(codec_base = quicktime_new_codec()) ) return 0;
		vtrack->codec = codec_base;
		vcodecs[i].init_vcodec(vtrack);
		if( quicktime_match_32(fourcc, codec_base->fourcc) ) {
			result = codec_base->title;
			done = 1;
		}
		codec_base->delete_vcodec(vtrack);
		quicktime_del_codec(codec_base);
	}

	free(vtrack);
	return result ? result : fourcc;
}

