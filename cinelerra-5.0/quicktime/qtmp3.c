#include "funcprotos.h"
#include "lame.h"
#include "libzmpeg3.h"
#include "quicktime.h"
#include "qtmp3.h"
#include <string.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define CLAMP(x, y, z) ((x) = ((x) <  (y) ? (y) : ((x) > (z) ? (z) : (x))))

#define OUTPUT_ALLOCATION 0x100000

//static FILE *test = 0;

typedef struct
{
// mp3 decoder
	mpeg3_layer_t *mp3;
// Number of first sample in output relative to file
	int64_t output_position;
// Current reading position in file
	int64_t chunk;
	int decode_initialized;
	float **output;

// mp3 encoder
	lame_global_flags *lame_global;
// This calculates the number of samples per chunk
	mpeg3_layer_t *encoded_header;
	int encode_initialized;
	float **input;
	int input_size;
	int input_allocated;
	int bitrate;
	unsigned char *encoder_output;
	int encoder_output_size;
	int encoder_output_allocated;
} quicktime_mp3_codec_t;

static void delete_codec(quicktime_audio_map_t *atrack)
{
	int i;
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if( codec->mp3 ) mpeg3_delete_layer(codec->mp3);
	if( codec->output ) {
		for( i=0; i<atrack->channels; ++i ) free(codec->output[i]);
		free(codec->output);
	}
	if( codec->lame_global )
		lame_close(codec->lame_global);
	if( codec->input ) {
		for( i=0; i<atrack->channels; ++i ) free(codec->input[i]);
		free(codec->input);
	}
	if( codec->encoder_output ) free(codec->encoder_output);
	if( codec->encoded_header )
		mpeg3_delete_layer(codec->encoded_header);
	free(codec);
}

static int decode(quicktime_t *file,
	int16_t *output_i, float *output_f, 
	long samples, int track, int channel)
{
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_vbr_t *vbr = &track_map->vbr;
	int channels = track_map->channels;
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int64_t current_position = track_map->current_position;
	int64_t end_position = current_position + samples;
	int i, retry = 0, result = 0;

	if(output_i) bzero(output_i, sizeof(int16_t) * samples);
	if(output_f) bzero(output_f, sizeof(float) * samples);

	if( quicktime_limit_samples(samples) ) return 1;

// Initialize and load initial buffer for decoding
	if( !codec->decode_initialized ) {
		codec->decode_initialized = 1;
		quicktime_init_vbr(vbr, channels);
		codec->output = malloc(sizeof(float*) * track_map->channels);
		for(i = 0; i < track_map->channels; i++)
			codec->output[i] = malloc(sizeof(float) * MAXFRAMESAMPLES);
		codec->mp3 = mpeg3_new_layer();
	}

	if( quicktime_align_vbr(track_map, current_position) ||
	    codec->output_position != quicktime_vbr_output_end(vbr) ) {
		quicktime_seek_vbr(track_map, current_position, 1);
		mpeg3_layer_reset(codec->mp3);
		codec->output_position = quicktime_vbr_output_end(vbr);
	}

	retry = 64;
	while( quicktime_vbr_output_end(vbr) < end_position && retry >= 0 ) {
// find a frame header
		int bfr_sz = quicktime_vbr_input_size(vbr);
		unsigned char *bfr = quicktime_vbr_input(vbr);
		int frame_size = 0;
		while( bfr_sz >= 4 ) {
			frame_size = mpeg3_layer_header(codec->mp3, bfr);
			if( frame_size ) break;
			++bfr;  --bfr_sz;
		}
		if( !frame_size ) {
			if( quicktime_read_vbr(file, track_map) ) break;
			--retry;  continue;
		}

// shift out data before frame header
		i = bfr - quicktime_vbr_input(vbr);
		if( i > 0 ) quicktime_shift_vbr(track_map, i);

// collect frame data
		while( quicktime_vbr_input_size(vbr) < frame_size && retry >= 0 ) {
			if( quicktime_read_vbr(file, track_map) ) break;
			--retry;
		}
		if( retry < 0  ) break;
		if( quicktime_vbr_input_size(vbr) < frame_size ) break;

// decode frame
		result = mpeg3audio_dolayer3(codec->mp3, 
				(char *)quicktime_vbr_input(vbr), frame_size,
				codec->output, 1);
		if( result > 0 ) {
                	quicktime_store_vbr_floatp(track_map, codec->output, result);
			codec->output_position += result;
			retry = 64;
		}
		quicktime_shift_vbr(track_map, frame_size);
	}

	if(output_i)
		quicktime_copy_vbr_int16(vbr, current_position,
			samples, output_i, channel);
	if(output_f)
                quicktime_copy_vbr_float(vbr, current_position,
			samples, output_f, channel);
	return 0;
}




static int allocate_output(quicktime_mp3_codec_t *codec,
	int samples)
{
	int new_size = codec->encoder_output_size + samples * 4;
	if(codec->encoder_output_allocated < new_size)
	{
		unsigned char *new_output = calloc(1, new_size);

		if(codec->encoder_output)
		{
			memcpy(new_output, 
				codec->encoder_output, 
				codec->encoder_output_size);
			free(codec->encoder_output);
		}
		codec->encoder_output = new_output;
		codec->encoder_output_allocated = new_size;
	}
	return 0;
}



// Empty the output buffer of frames
static int write_frames(quicktime_t *file, 
	quicktime_audio_map_t *track_map,
	quicktime_trak_t *trak,
	quicktime_mp3_codec_t *codec,
	int track)
{
	int frame_end = codec->encoder_output_size;
	unsigned char *bfr = codec->encoder_output;
	int frame_limit = frame_end - 4;
	int frame_samples;
	int result = 0;
	int i = 0;
	
// Write to chunks
	while( !result && i < frame_limit ) {
		char *hdr = (char *)bfr + i;
		int frame_size = mpeg3_layer_header(codec->encoded_header,
			(unsigned char *)hdr);
// Not the start of a frame.  Skip it, try next bfr byte;
		if( !frame_size ) { ++i;  continue; }
// Frame is finished before end of buffer
		if( i+frame_size > frame_end ) break;
		i += frame_size;
// Write the chunk
		frame_samples = mpeg3audio_dolayer3(codec->encoded_header, 
			hdr, frame_size, 0, 0);
// Frame is a dud
		if( !frame_samples ) continue;
		result = quicktime_write_vbr_frame(file, track,
			hdr, frame_size, frame_samples);
		track_map->current_chunk++;
	}

// move any frame fragment down
	if( i > 0 ) {
		int j = 0;
		codec->encoder_output_size -= i;
		while( i < frame_end ) bfr[j++] = bfr[i++];
	}

	return result;
}






static int encode(quicktime_t *file, 
	int16_t **input_i, float **input_f, int track, long samples)
{
	int result = 0;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int new_size = codec->input_size + samples;
	int i, j;

	if(!codec->encode_initialized)
	{
		codec->encode_initialized = 1;
		codec->lame_global = lame_init();
		lame_set_brate(codec->lame_global, codec->bitrate / 1000);
		lame_set_quality(codec->lame_global, 0);
		lame_set_in_samplerate(codec->lame_global, 
			trak->mdia.minf.stbl.stsd.table[0].sample_rate);
		if((result = lame_init_params(codec->lame_global)) < 0)
			printf("encode: lame_init_params returned %d\n", result);
		codec->encoded_header = mpeg3_new_layer();
		if(file->use_avi)
			trak->mdia.minf.stbl.stsd.table[0].sample_size = 0;
	}


// Stack input on end of buffer
	if(new_size > codec->input_allocated)
	{
		float *new_input;

		if(!codec->input) 
			codec->input = calloc(sizeof(float*), track_map->channels);

		for(i = 0; i < track_map->channels; i++)
		{
			new_input = calloc(sizeof(float), new_size);
			if(codec->input[i])
			{
				memcpy(new_input, codec->input[i], sizeof(float) * codec->input_size);
				free(codec->input[i]);
			}
			codec->input[i] = new_input;
		}
		codec->input_allocated = new_size;
	}


// Transfer to input buffers
	if(input_i)
	{
		for(i = 0; i < track_map->channels; i++)
		{
			for(j = 0; j < samples; j++)
				codec->input[i][j] = input_i[i][j];
		}
	}
	else
	if(input_f)
	{
		for(i = 0; i < track_map->channels; i++)
		{
			for(j = 0; j < samples; j++)
				codec->input[i][j] = input_f[i][j] * 32767;
		}
	}

// Encode
	allocate_output(codec, samples);

	result = lame_encode_buffer_float(codec->lame_global,
		codec->input[0],
		(track_map->channels > 1) ? codec->input[1] : codec->input[0],
		samples,
		codec->encoder_output + codec->encoder_output_size,
		codec->encoder_output_allocated - codec->encoder_output_size);

	codec->encoder_output_size += result;

	result = write_frames(file, track_map, trak, codec, track);
	return result;
}



static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(!strcasecmp(key, "mp3_bitrate"))
		codec->bitrate = *(int*)value;

	return 0;
}



static void flush(quicktime_t *file, int track)
{
	int result = 0;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

	if(codec->encode_initialized)
	{
		result = lame_encode_flush(codec->lame_global,
        	codec->encoder_output + codec->encoder_output_size, 
			codec->encoder_output_allocated - codec->encoder_output_size);
		codec->encoder_output_size += result;
		result = write_frames(file, track_map, trak, codec, track);
	}
}


void quicktime_init_codec_mp3(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_mp3_codec_t *codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_mp3_codec_t));
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_audio = decode;
	codec_base->encode_audio = encode;
	codec_base->set_parameter = set_parameter;
	codec_base->flush = flush;
	codec_base->fourcc = QUICKTIME_MP3;
	codec_base->title = "MP3";
	codec_base->desc = "MP3 for video";
	codec_base->wav_id = 0x55;

	codec = codec_base->priv;
	codec->bitrate = 256000;
}




