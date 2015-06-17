#include "colormodels.h"
#include "funcprotos.h"
#include "quicktime.h"
#include "qtffmpeg.h"
#include "qtprivate.h"

#include <string.h>
// FFMPEG front end for quicktime.

int ffmpeg_initialized = 0;
pthread_mutex_t ffmpeg_lock = PTHREAD_MUTEX_INITIALIZER;

quicktime_ffmpeg_t *quicktime_new_ffmpeg(int cpus,
		 int fields, int ffmpeg_id, int w, int h,
		 quicktime_stsd_table_t *stsd_table)
{
	quicktime_ffmpeg_t *ptr = calloc(1, sizeof(quicktime_ffmpeg_t));
	quicktime_esds_t *esds = &stsd_table->esds;
	quicktime_avcc_t *avcc = &stsd_table->avcc;
	int i;

	ptr->fields = fields;
	ptr->width = w;
	ptr->height = h;
	ptr->ffmpeg_id = ffmpeg_id;

	if( ffmpeg_id == CODEC_ID_SVQ1 ) {
		ptr->width_i = quicktime_quantize32(ptr->width);
		ptr->height_i = quicktime_quantize32(ptr->height);
	}
	else {
		ptr->width_i = quicktime_quantize16(ptr->width);
		ptr->height_i = quicktime_quantize16(ptr->height);
	}

	pthread_mutex_lock(&ffmpeg_lock);
	if( !ffmpeg_initialized ) {
		ffmpeg_initialized = 1;
		av_register_all();
	}

	for( i=0; i<fields; ++i ) {
		ptr->decoder[i] = avcodec_find_decoder(ptr->ffmpeg_id);
		if( !ptr->decoder[i] ) {
			printf("quicktime_new_ffmpeg: avcodec_find_decoder returned NULL.\n");
			quicktime_delete_ffmpeg(ptr);
			return 0;
		}

		AVCodecContext *context = avcodec_alloc_context3(ptr->decoder[i]);
		ptr->decoder_context[i] = context;
		static char fake_data[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//		static unsigned char extra_data[] = {
//			0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x20,
//			0x00, 0xc8, 0x88, 0xba, 0x98, 0x60, 0xfa, 0x67,
//			0x80, 0x91, 0x02, 0x83, 0x1f };
		context->width = ptr->width_i;
		context->height = ptr->height_i;
//		context->width = w;
//		context->height = h;
		context->extradata = (unsigned char *) fake_data;
		context->extradata_size = 0;
		if( esds->mpeg4_header && esds->mpeg4_header_size ) {
			context->extradata = (unsigned char *) esds->mpeg4_header;
			context->extradata_size = esds->mpeg4_header_size;
		}
		else if( avcc->data && avcc->data_size ) {
			context->extradata = (unsigned char *) avcc->data;
			context->extradata_size = avcc->data_size;
		}
		int result = -1;
		if( cpus > 1 ) {
			context->thread_count = cpus;
			result = avcodec_open2(context, ptr->decoder[i], 0);
		}
		if( result < 0 ) {
			context->thread_count = 1;
			result = avcodec_open2(context, ptr->decoder[i], 0);
		}
		if( result < 0 ) {
			printf("quicktime_new_ffmpeg: avcodec_open failed.\n");
			quicktime_delete_ffmpeg(ptr);
			ptr = NULL;
			break;
		}

		ptr->last_frame[i] = -1;
	}

	pthread_mutex_unlock(&ffmpeg_lock);
	return ptr;
}

void quicktime_delete_ffmpeg(quicktime_ffmpeg_t *ptr)
{
	int i;
	if( !ptr ) return;
	pthread_mutex_lock(&ffmpeg_lock);
	for( i=0; i<ptr->fields; ++i ) {
		if( !ptr->decoder_context[i] ) continue;
    		avcodec_close(ptr->decoder_context[i]);
		free(ptr->decoder_context[i]);
	}
	pthread_mutex_unlock(&ffmpeg_lock);

	if(ptr->temp_frame) free(ptr->temp_frame);
	if(ptr->work_buffer) free(ptr->work_buffer);
	free(ptr);
}

//avcodec_get_frame_defaults
static void frame_defaults(AVFrame *frame)
{
	memset(frame, 0, sizeof(AVFrame));
	av_frame_unref(frame);
}

static int decode_wrapper(quicktime_t *file,
		 quicktime_video_map_t *vtrack, quicktime_ffmpeg_t *ffmpeg,
		 int frame_number, int current_field, int track, int drop_it)
{
	quicktime_trak_t *trak = vtrack->track;
	quicktime_stsd_table_t *stsd_table = &trak->mdia.minf.stbl.stsd.table[0];
	int got_picture = 0, result = 0;
	int header_bytes = !frame_number ? stsd_table->esds.mpeg4_header_size : 0;
	int bytes;  char *data;
	quicktime_set_video_position(file, frame_number, track);
	bytes = quicktime_frame_size(file, frame_number, track) + header_bytes;

	if( ffmpeg->work_buffer && ffmpeg->buffer_size < bytes ) {
		free(ffmpeg->work_buffer);  ffmpeg->work_buffer = 0;
	}
	if( !ffmpeg->work_buffer ) {
		ffmpeg->buffer_size = bytes;
		ffmpeg->work_buffer = calloc(1, ffmpeg->buffer_size + 100);
	}
	data = (char *)ffmpeg->work_buffer;
	if( header_bytes ) {
		memcpy(data, stsd_table->esds.mpeg4_header, header_bytes);
		data += header_bytes;  bytes -= header_bytes;
		header_bytes = 0;
	}

	if( !quicktime_read_data(file, data, bytes) )
		result = -1;

	AVPacket avpkt;
	av_init_packet(&avpkt);
	avpkt.data = ffmpeg->work_buffer;
	avpkt.size = bytes;
	avpkt.flags = AV_PKT_FLAG_KEY;

	while( !result && !got_picture && avpkt.size > 0 ) {
		ffmpeg->decoder_context[current_field]->skip_frame =
			drop_it ?  AVDISCARD_NONREF : AVDISCARD_DEFAULT;
		frame_defaults(&ffmpeg->picture[current_field]);
		ffmpeg->decoder_context[current_field]->workaround_bugs = FF_BUG_NO_PADDING;
		result = avcodec_decode_video2(ffmpeg->decoder_context[current_field],
				 &ffmpeg->picture[current_field], &got_picture, &avpkt);
		if( result < 0 ) break;
		avpkt.data += result;
		avpkt.size -= result;
	}

#ifdef ARCH_X86
	asm("emms");
#endif
	return !got_picture;
}

// Get amount chroma planes are downsampled from luma plane.
// Used for copying planes into cache.
static int get_chroma_factor(quicktime_ffmpeg_t *ffmpeg, int current_field)
{
	switch(ffmpeg->decoder_context[current_field]->pix_fmt)
	{
		case PIX_FMT_YUV420P:  return 4;
		case PIX_FMT_YUVJ420P: return 4;
		case PIX_FMT_YUYV422:  return 2;
		case PIX_FMT_YUV422P:  return 2;
		case PIX_FMT_YUV410P:  return 9;
		default:
			fprintf(stderr, "get_chroma_factor: unrecognized color model %d\n",
				ffmpeg->decoder_context[current_field]->pix_fmt);
			break;
	}
	return 9;
}

int quicktime_ffmpeg_decode(quicktime_ffmpeg_t * ffmpeg,
		quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	int current_field = vtrack->current_position % ffmpeg->fields;
	int input_cmodel = BC_TRANSPARENCY;
	int i, seeking_done = 0;
	int result = quicktime_get_frame(vtrack->frame_cache, vtrack->current_position,
			 &ffmpeg->picture[current_field].data[0],
			 &ffmpeg->picture[current_field].data[1],
			 &ffmpeg->picture[current_field].data[2]);

	if( !result ) {
		if( ffmpeg->last_frame[current_field] == -1 && ffmpeg->ffmpeg_id != CODEC_ID_H264 ) {
			int current_frame = vtrack->current_position;
			result = decode_wrapper(file, vtrack, ffmpeg,
					current_field, current_field, track, 0);
			quicktime_set_video_position(file, current_frame, track);
			ffmpeg->last_frame[current_field] = current_field;
		}

		if( quicktime_has_keyframes(file, track) &&
		    vtrack->current_position != ffmpeg->last_frame[current_field] + ffmpeg->fields &&
		    vtrack->current_position != ffmpeg->last_frame[current_field]) {
			int frame1;
			int first_frame;
			int frame2 = vtrack->current_position;
			int current_frame = frame2;

			if( !quicktime_has_frame(vtrack->frame_cache, vtrack->current_position + 1) )
				quicktime_reset_cache(vtrack->frame_cache);

			frame1 = current_frame;
			do {
				frame1 = quicktime_get_keyframe_before(file, frame1 - 1, track);
			} while( frame1 > 0 && (frame1 % ffmpeg->fields) != current_field );

			if( 0 /* frame1 > 0 && ffmpeg->ffmpeg_id == CODEC_ID_MPEG4 */ ) {
				do {
					frame1 = quicktime_get_keyframe_before(file, frame1 - 1, track);
				} while( frame1 > 0 && (frame1 & ffmpeg->fields) != current_field );
			}

			if( frame1 < ffmpeg->last_frame[current_field] &&
			    frame2 > ffmpeg->last_frame[current_field]) {
				frame1 = ffmpeg->last_frame[current_field] + ffmpeg->fields;
			}
/*
 * printf("quicktime_ffmpeg_decode 2 last_frame=%d frame1=%d frame2=%d\n",
 * ffmpeg->last_frame[current_field], frame1, frame2);
 */
			first_frame = frame1;
			while( frame1 <= frame2 ) {
				result = decode_wrapper(file, vtrack, ffmpeg,
					frame1, current_field, track, 0 /* (frame1 < frame2) */ );
				if( ffmpeg->picture[current_field].data[0] && frame1 > first_frame ) {
					int y_size = ffmpeg->picture[current_field].linesize[0] * ffmpeg->height_i;
					int u_size = y_size / get_chroma_factor(ffmpeg, current_field);
					int v_size = y_size / get_chroma_factor(ffmpeg, current_field);
					quicktime_put_frame(vtrack->frame_cache, frame1,
						ffmpeg->picture[current_field].data[0],
						ffmpeg->picture[current_field].data[1],
						ffmpeg->picture[current_field].data[2],
						y_size, u_size, v_size);
				}

				frame1 += ffmpeg->fields;
			}

			vtrack->current_position = frame2;
			seeking_done = 1;
		}

		if (!seeking_done && vtrack->current_position != ffmpeg->last_frame[current_field]) {
			result = decode_wrapper(file, vtrack, ffmpeg,
				vtrack->current_position, current_field, track, 0);
		}

		ffmpeg->last_frame[current_field] = vtrack->current_position;
	}

	switch (ffmpeg->decoder_context[current_field]->pix_fmt) {
	case PIX_FMT_YUV420P:  input_cmodel = BC_YUV420P; break;
	case PIX_FMT_YUYV422:  input_cmodel = BC_YUV422;  break;
	case PIX_FMT_YUVJ420P: input_cmodel = BC_YUV420P; break;
	case PIX_FMT_YUV422P:  input_cmodel = BC_YUV422P; break;
	case PIX_FMT_YUV410P:  input_cmodel = BC_YUV9P;   break;
	default:
		input_cmodel = 0;
		if (!ffmpeg->picture[current_field].data[0])
			break;
		fprintf(stderr, "quicktime_ffmpeg_decode: unrecognized color model %d\n",
				ffmpeg->decoder_context[current_field]->pix_fmt);
		break;
	}

	if (ffmpeg->picture[current_field].data[0]) {
		AVCodecContext *ctx = ffmpeg->decoder_context[current_field];
		int width = ctx->width, height = ctx->height;
		AVFrame *picture = &ffmpeg->picture[current_field];
		unsigned char *data = picture->data[0];
		unsigned char **input_rows = malloc(sizeof(unsigned char *) * height);
		int line_sz = cmodel_calculate_pixelsize(input_cmodel) * width;
		for( i=0; i<height; ++i ) input_rows[i] = data + i * line_sz;
		cmodel_transfer(row_pointers, input_rows,
				row_pointers[0], row_pointers[1], row_pointers[2],
				picture->data[0], picture->data[1], picture->data[2],
				file->in_x, file->in_y, file->in_w, file->in_h,
				0, 0, file->out_w, file->out_h,
				input_cmodel, file->color_model,
				0, /* BC_RGBA8888 to non-alpha background color */
				ffmpeg->picture[current_field].linesize[0],
				ffmpeg->width); /* For planar use the luma rowspan */
		free(input_rows);
	}

	return result;
}


/* assumes 16-bit, interleaved data */
/* always moves buffer */
int quicktime_decode_audio3(
		AVCodecContext *avctx, int16_t *samples,
		int *frame_size_ptr, AVPacket *avpkt)
{
	int ret, got_frame = 0;
	AVFrame *frame = av_frame_alloc();
	if (!frame) return -1;

	ret = avcodec_decode_audio4(avctx, frame, &got_frame, avpkt);

	if( ret >= 0 && got_frame ) {
		int plane_size;
		int data_size = av_samples_get_buffer_size(&plane_size, avctx->channels,
					   frame->nb_samples, avctx->sample_fmt, 1);
		if( *frame_size_ptr < data_size ) {
			printf("quicktime_decode_audio3: output buffer size is too small for "
				"the current frame (%d < %d)\n", *frame_size_ptr, data_size);
			av_frame_free(&frame);
			return -1;
		}
		memcpy(samples, frame->extended_data[0], plane_size);
		*frame_size_ptr = data_size;
	} else {
		*frame_size_ptr = 0;
	}
	av_frame_free(&frame);
	return ret;
}

