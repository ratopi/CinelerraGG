#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "yuv2.h"

/* U V values are signed but Y R G B values are unsigned! */
/*
 *      R = Y               + 1.40200 * V
 *      G = Y - 0.34414 * U - 0.71414 * V
 *      B = Y + 1.77200 * U
 */

/*
 *		Y =  0.2990 * R + 0.5870 * G + 0.1140 * B
 *		U = -0.1687 * R - 0.3310 * G + 0.5000 * B
 *		V =  0.5000 * R - 0.4187 * G - 0.0813 * B
 */


typedef struct
{
	unsigned char *work_buffer;
	int coded_w, coded_h;

/* The YUV2 codec requires a bytes per line that is a multiple of 4 */
	int bytes_per_line;
	int initialized;

        int is_2vuy;
        uint8_t ** rows;
  } quicktime_yuv2_codec_t;

static void quicktime_delete_codec_yuv2(quicktime_video_map_t *vtrack)
{
	quicktime_yuv2_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
        if(codec->rows) free(codec->rows);
        free(codec);
}
#if 0
static int quicktime_reads_colormodel_yuv2(quicktime_t *file,
		int colormodel,
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUV422P);
}
#endif

static void convert_encode_yuv2(quicktime_yuv2_codec_t *codec)
{
	int y, x;
	for(y = 0; y < codec->coded_h; y++) {
		unsigned char *row = codec->work_buffer + y * codec->bytes_per_line;
		for(x = 0; x < codec->bytes_per_line; ) {
			row[1] -= 128;
			row[3] -= 128;
			row += 4;
			x += 4;
		}
	}
}

static void convert_decode_yuv2(quicktime_yuv2_codec_t *codec)
{
	int y, x;
	for(y = 0; y < codec->coded_h; y++) {
		unsigned char *row = codec->work_buffer + y * codec->bytes_per_line;
		for(x = 0; x < codec->bytes_per_line; ) {
			row[1] += 128;
			row[3] += 128;
			row += 4;
			x += 4;
		}
	}
}

static void convert_encode_2vuy(quicktime_yuv2_codec_t *codec)
{
	int y, x;
	for(y = 0; y < codec->coded_h; y++) {
		unsigned char *row = codec->work_buffer + y * codec->bytes_per_line;
		for(x = 0; x < codec->bytes_per_line; ) {
			int y0 = row[0], u = row[1];
			int y1 = row[2], v = row[3];
			row[0] = u;  row[1] = y0;
			row[2] = v;  row[3] = y1;
                        row += 4;
			x += 4;
		}
	}
}

static void convert_decode_2vuy(quicktime_yuv2_codec_t *codec)
{
        int y, x;
	for(y = 0; y < codec->coded_h; y++) {
		unsigned char *row = codec->work_buffer + y * codec->bytes_per_line;
		for(x = 0; x < codec->bytes_per_line; ) {
			int u = row[0], y0 = row[1];
			int v = row[2], y1 = row[3];
			row[0] = y0;  row[1] = u;
			row[2] = y1;  row[3] = v;
			row += 4;
                        x += 4;
		}
	}
}


static void initialize(quicktime_video_map_t *vtrack, quicktime_yuv2_codec_t *codec,
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
	quicktime_yuv2_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	int result = 0;

	initialize(vtrack, codec, width, height);
	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);

        result = !quicktime_read_data(file, (char*)codec->work_buffer, bytes);
	if(codec->is_2vuy)
		convert_decode_2vuy(codec);
	else
		convert_decode_yuv2(codec);

	cmodel_transfer(row_pointers, codec->rows,
		row_pointers[0], row_pointers[1], row_pointers[2],
		0, 0, 0,
		file->in_x, file->in_y, file->in_w, file->in_h,
		0, 0, file->out_w, file->out_h,
		BC_YUV422, file->color_model,
		0, codec->coded_w, file->out_w);

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int bytes;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_yuv2_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
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
		file->color_model, BC_YUV422,
		0, width, codec->coded_w);

	if( codec->is_2vuy )
       		convert_encode_2vuy(codec);
	else
		convert_encode_yuv2(codec);

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

void quicktime_init_codec_yuv2(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_yuv2_codec_t));
	codec_base->delete_vcodec = quicktime_delete_codec_yuv2;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->fourcc = QUICKTIME_YUV2;
	codec_base->title = "Component Y'CbCr 8-bit 4:2:2 (yuv2)";
	codec_base->desc = "YUV 4:2:2";
}

void quicktime_init_codec_2vuy(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
        quicktime_yuv2_codec_t * codec;
/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_yuv2_codec_t));
	codec_base->delete_vcodec = quicktime_delete_codec_yuv2;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->fourcc = QUICKTIME_2VUY;
	codec_base->title = "Component Y'CbCr 8-bit 4:2:2 (2vuy)";
	codec_base->desc = "YUV 4:2:2";
        codec = (quicktime_yuv2_codec_t *)(codec_base->priv);
        codec->is_2vuy = 1;
}

