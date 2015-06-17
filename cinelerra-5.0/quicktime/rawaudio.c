#include "quicktime.h"
#include "rawaudio.h"
#include <math.h>

typedef struct
{
	char *work_buffer;
	long buffer_size;
} quicktime_rawaudio_codec_t;

/* =================================== private for rawaudio */

static int is_little_endian(void)
{
	int16_t test = 0x0001;
	return *((unsigned char *)&test);
}

static int get_work_buffer(quicktime_t *file, int track, long bytes)
{
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

	if(codec->work_buffer && codec->buffer_size != bytes) {
		free(codec->work_buffer);
		codec->work_buffer = 0;
	}
	
	if(!codec->work_buffer) {
		codec->buffer_size = bytes;
		if(!(codec->work_buffer = malloc(bytes))) return 1;
	}
	return 0;
}

/* =================================== public for rawaudio */

static void quicktime_delete_codec_rawaudio(quicktime_audio_map_t *atrack)
{
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->work_buffer) free(codec->work_buffer);
	codec->work_buffer = 0;
	codec->buffer_size = 0;
	free(codec);
}

static int fclip(float f, int mx)
{
  f *= mx;  f += f >= 0 ? 0.5f : -0.5f;
  return f >= mx ? mx+mx-1 : f < -mx ? 0 : (int)f + mx;
}
static int fclip8 (float f) { return fclip(f,0x80); }
static int fclip16(float f) { return fclip(f,0x8000); }
static int fclip24(float f) { return fclip(f,0x800000); }

static int   rd8be_s16 (uint8_t *bp) { return (bp[0]<<8) - 0x8000; }
static int   rd8le_s16 (uint8_t *bp) { return (bp[0]<<8) - 0x8000; }
static int   rd16be_s16(uint8_t *bp) { return ((bp[0]<<8) | bp[1]) - 0x8000; }
static int   rd16le_s16(uint8_t *bp) { return (bp[0] | (bp[1]<<8)) - 0x8000; }
static int   rd24be_s16(uint8_t *bp) { return ((bp[0]<<8) | bp[1]) - 0x8000; }
static int   rd24le_s16(uint8_t *bp) { return (bp[1] | (bp[2]<<8)) - 0x8000; }
static float rd8be_flt (uint8_t *bp) { return (float)(bp[0]-0x80)/0x80; }
static float rd8le_flt (uint8_t *bp) { return (float)(bp[0]-0x80)/0x80; }
static float rd16be_flt(uint8_t *bp) { return (float)(((bp[0]<<8) | bp[1])-0x8000)/0x8000; }
static float rd16le_flt(uint8_t *bp) { return (float)((bp[0] | (bp[1]<<8))-0x8000)/0x8000; }
static float rd24be_flt(uint8_t *bp) { return (float)(((bp[0]<<16) | (bp[1]<<8) | bp[2])-0x800000)/0x800000; }
static float rd24le_flt(uint8_t *bp) { return (float)((bp[0] | (bp[1]<<8) | (bp[2]<<16))-0x800000)/0x800000; }

static void wr8be_s16 (uint8_t *bp, int v)   { v+=0x8000; bp[0] = v>>8; }
static void wr8le_s16 (uint8_t *bp, int v)   { v+=0x8000; bp[0] = v>>8; }
static void wr16be_s16(uint8_t *bp, int v)   { v+=0x8000; bp[0] = v>>8; bp[1] = v; }
static void wr16le_s16(uint8_t *bp, int v)   { v+=0x8000; bp[0] = v; bp[1] = v>>8; }
static void wr24be_s16(uint8_t *bp, int v)   { v+=0x8000; bp[0] = v>>8; bp[1] = v; bp[2] = 0; }
static void wr24le_s16(uint8_t *bp, int v)   { v+=0x8000; bp[0] = 0; bp[1] = v; bp[2] = v>>8; }
static void wr8be_flt (uint8_t *bp, float f) { int v = fclip8(f); bp[0] = v; }
static void wr8le_flt (uint8_t *bp, float f) { int v = fclip8(f); bp[0] = v; }
static void wr16be_flt(uint8_t *bp, float f) { int v = fclip16(f); bp[0] = v>>8; bp[1] = v; }
static void wr16le_flt(uint8_t *bp, float f) { int v = fclip16(f); bp[0] = v; bp[1] = v>>8; }
static void wr24be_flt(uint8_t *bp, float f) { int v = fclip24(f); bp[0] = v>>16; bp[1] = v>>8; bp[2] = v; }
static void wr24le_flt(uint8_t *bp, float f) { int v = fclip24(f); bp[0] = v; bp[1] = v>>8; bp[2] = v>>16; }

#define rd_samples(typ, fn, out, step) { \
  uint8_t *bp = (uint8_t *)codec->work_buffer + channel*byts; \
  for( i=0; i<samples; ++i ) { *out++ = fn(bp); bp += step; } \
}

#define wr_samples(typ, fn, in, step) { \
  for( j=0; j<channels; ++j ) { typ *inp = in[j]; \
    uint8_t *bp = (uint8_t *)codec->work_buffer + j*byts; \
    for( i=0; i<samples; ++i ) { fn(bp,*inp++); bp += step; } \
  } \
}

static int quicktime_decode_rawaudio(quicktime_t *file, 
		int16_t *output_i, float *output_f, long samples, 
		int track, int channel)
{
	int i, result;
	int little_endian = is_little_endian();
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int bits = quicktime_audio_bits(file, track);
	int byts = bits/8, channels = file->atracks[track].channels;
	int size = channels * byts;
	get_work_buffer(file, track, samples * size);
	result = !quicktime_read_audio(file, codec->work_buffer, samples, track);
	if( result ) return result;
// Undo increment since this is done in codecs.c
	track_map->current_position -= samples;
	if( !little_endian ) {
		if( output_i ) {
			switch( byts ) {
			case 1:  rd_samples(int16_t,rd8be_s16,output_i,size);   break;
			case 2:  rd_samples(int16_t,rd16be_s16,output_i,size);  break;
			case 3:  rd_samples(int16_t,rd24be_s16,output_i,size);  break;
			}
		}
		else if( output_f ) {
			switch( byts ) {
			case 1:  rd_samples(float,rd8be_flt,output_f,size);   break;
			case 2:  rd_samples(float,rd16be_flt,output_f,size);  break;
			case 3:  rd_samples(float,rd24be_flt,output_f,size);  break;
			}
		}
	}
	else {
// Undo increment since this is done in codecs.c
		track_map->current_position -= samples;
		if( output_i ) {
			switch( byts ) {
			case 1:  rd_samples(int16_t,rd8le_s16,output_i,size);   break;
			case 2:  rd_samples(int16_t,rd16le_s16,output_i,size);  break;
			case 3:  rd_samples(int16_t,rd24le_s16,output_i,size);  break;
			}
		}
		else if( output_f ) {
			switch( byts ) {
			case 1:  rd_samples(float,rd8le_flt,output_f,size);   break;
			case 2:  rd_samples(float,rd16le_flt,output_f,size);  break;
			case 3:  rd_samples(float,rd24le_flt,output_f,size);  break;
			}
		}
	}
/*printf("quicktime_decode_rawaudio 2\n"); */
	return 0;
}

static int quicktime_encode_rawaudio(quicktime_t *file, 
		int16_t **input_i, float **input_f, int track, long samples)
{
	int i, j, result;
	int little_endian = is_little_endian();
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int bits = quicktime_audio_bits(file, track);
	int byts = bits/8, channels = file->atracks[track].channels;
	int size = channels * byts;
	get_work_buffer(file, track, samples * size);

	if( !little_endian ) {
		if( input_i ) {
			switch( byts ) {
			case 1:  wr_samples(int16_t,wr8be_s16,input_i,size);   break;
			case 2:  wr_samples(int16_t,wr16be_s16,input_i,size);  break;
			case 3:  wr_samples(int16_t,wr24be_s16,input_i,size);  break;
			}
		}
		else if( input_f ) {
			switch( byts ) {
			case 1:  wr_samples(float,wr8be_flt,input_f,size);   break;
			case 2:  wr_samples(float,wr16be_flt,input_f,size);  break;
			case 3:  wr_samples(float,wr24be_flt,input_f,size);  break;
			}
		}
	}
	else {
		if( input_i ) {
			switch( byts ) {
			case 1:  wr_samples(int16_t,wr8le_s16,input_i,size);   break;
			case 2:  wr_samples(int16_t,wr16le_s16,input_i,size);  break;
			case 3:  wr_samples(int16_t,wr24le_s16,input_i,size);  break;
			}
		}
		else if( input_f ) {
			switch( byts ) {
			case 1:  wr_samples(float,wr8le_flt,input_f,size);   break;
			case 2:  wr_samples(float,wr16le_flt,input_f,size);  break;
			case 3:  wr_samples(float,wr24le_flt,input_f,size);  break;
			}
		}
	}

	result = quicktime_write_audio(file, (char *)codec->work_buffer, samples, track);
	return result;
}


void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
//	quicktime_rawaudio_codec_t *codec = codec_base->priv;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_rawaudio_codec_t));
	codec_base->delete_acodec = quicktime_delete_codec_rawaudio;
	codec_base->decode_video = 0;
	codec_base->encode_video = 0;
	codec_base->decode_audio = quicktime_decode_rawaudio;
	codec_base->encode_audio = quicktime_encode_rawaudio;
	codec_base->fourcc = QUICKTIME_RAW;
	codec_base->title = "8 bit unsigned";
	codec_base->desc = "8 bit unsigned for audio";
	codec_base->wav_id = 0x01;
/* Init private items */
}
