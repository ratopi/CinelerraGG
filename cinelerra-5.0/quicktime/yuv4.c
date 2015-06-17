#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "yuv4.h"

typedef struct
{
	unsigned char *work_buffer;
	int coded_w, coded_h;

	int bytes_per_line;
	int initialized;

        uint8_t ** rows;
  } quicktime_yuv4_codec_t;

static void quicktime_delete_codec_yuv4(quicktime_video_map_t *vtrack)
{
	quicktime_yuv4_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
        if(codec->rows) free(codec->rows);
        free(codec);
}

static void convert_encode_yuv4(quicktime_yuv4_codec_t *codec)
{
	int y, x;
	unsigned char *outp = codec->work_buffer;
	for(y = 0; y < codec->coded_h; y+=2) {
		int y1 = y+1 >= codec->coded_h ? y : y+1;
		unsigned char *row0 = codec->work_buffer + y * codec->bytes_per_line;
		unsigned char *row1 = codec->work_buffer + y1 * codec->bytes_per_line;
		for(x = 0; x < codec->bytes_per_line; x+=2) {
			int y00 = *row0++, u  = *row0++, v  = *row0++;
			int y01 = *row0++; u += *row0++, v += *row0++;
			int y10 = *row1++; u += *row1++, v += *row1++;
			int y11 = *row1++; u += *row1++, v += *row1++;
			*outp++ = u/4;  *outp++ = v/4;
			*outp++ = y00;  *outp++ = y01;
			*outp++ = y10;  *outp++ = y11;
		}
	}
}

static void convert_decode_yuv4(quicktime_yuv4_codec_t *codec)
{
	int y, x;
	unsigned char *inp = codec->work_buffer;
	for(y = 0; y < codec->coded_h; y+=2) {
		int y1 = y+1 >= codec->coded_h ? y : y+1;
		unsigned char *row0 = codec->work_buffer + y * codec->bytes_per_line;
		unsigned char *row1 = codec->work_buffer + y1 * codec->bytes_per_line;
		for(x = 0; x < codec->bytes_per_line; ) {
			int   u = *inp++,    v = *inp++;
			int y00 = *inp++,  y01 = *inp++;
			int y10 = *inp++,  y11 = *inp++;
			*row0++ = y00;  *row0++ = u;  *row0++ = v;
			*row0++ = y01;  *row0++ = u;  *row0++ = v;
			*row1++ = y10;  *row1++ = u;  *row1++ = v;
			*row1++ = y11;  *row1++ = u;  *row1++ = v;
		}
	}
}


static void initialize(quicktime_video_map_t *vtrack, quicktime_yuv4_codec_t *codec,
                       int width, int height)
{
	if(!codec->initialized)
	{
		int y;
/* Init private items */
		codec->coded_w = (int)((float)width / 4 + 0.5) * 4;
//		codec->coded_h = (int)((float)vtrack->track->tkhd.track_height / 4 + 0.5) * 4;
                codec->coded_h = height;
		codec->bytes_per_line = codec->coded_w * 2;
		codec->work_buffer = malloc(codec->bytes_per_line * codec->coded_h);
		codec->rows = malloc(height * sizeof(*(codec->rows)));
		for(y = 0; y < height; y++)
			codec->rows[y] = &codec->work_buffer[y * codec->bytes_per_line];
		codec->initialized = 1;
    }
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int bytes;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yuv4_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	int result = 0;

	initialize(vtrack, codec, width, height);
	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);

        result = !quicktime_read_data(file, (char*)codec->work_buffer, bytes);
	convert_decode_yuv4(codec);

	cmodel_transfer(row_pointers, codec->rows,
		row_pointers[0], row_pointers[1], row_pointers[2],
		0, 0, 0,
		file->in_x, file->in_y, file->in_w, file->in_h,
		0, 0, file->out_w, file->out_h,
		BC_YUV888, file->color_model,
		0, codec->coded_w, file->out_w);

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int bytes;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_yuv4_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int result = 1;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	quicktime_atom_t chunk_atom;
	initialize(vtrack, codec, width, height);
	bytes = height * codec->bytes_per_line;

	cmodel_transfer(codec->rows, row_pointers,
		0, 0, 0,
		row_pointers[0], row_pointers[1], row_pointers[2],
		0, 0, width, height,
		0, 0, width, height,
		file->color_model, BC_YUV888,
		0, width, codec->coded_w);

	convert_encode_yuv4(codec);

	quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, (char*)codec->work_buffer, bytes);
	quicktime_write_chunk_footer(file, trak,
		vtrack->current_chunk, &chunk_atom, 1);

	vtrack->current_chunk++;
	return result;
}

static int reads_colormodel(quicktime_t *file,
		int colormodel,
		int track)
{

	return (colormodel == BC_RGB888 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUV422P ||
		colormodel == BC_YUV422);
}

static int writes_colormodel(quicktime_t *file,
		int colormodel,
		int track)
{

	return (colormodel == BC_RGB888 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUV422P ||
		colormodel == BC_YUV422);
}


void quicktime_init_codec_yuv4(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_yuv4_codec_t));
	codec_base->delete_vcodec = quicktime_delete_codec_yuv4;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->fourcc = QUICKTIME_YUV4;
	codec_base->title = "YUV 4:2:0 packed";
	codec_base->desc = "YUV 4:2:0 packed (Not standardized)";
}


