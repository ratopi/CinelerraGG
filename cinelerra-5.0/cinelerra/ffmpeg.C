
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
// work arounds (centos)
#include <lzma.h>
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

#include "asset.h"
#include "bccmodels.h"
#include "fileffmpeg.h"
#include "file.h"
#include "ffmpeg.h"
#include "mwindow.h"
#include "vframe.h"


#define VIDEO_INBUF_SIZE 0x10000
#define AUDIO_INBUF_SIZE 0x10000
#define VIDEO_REFILL_THRESH 0
#define AUDIO_REFILL_THRESH 0x1000

Mutex FFMPEG::fflock("FFMPEG::fflock");

static void ff_err(int ret, const char *msg)
{
	char errmsg[BCSTRLEN];  av_strerror(ret, errmsg, sizeof(errmsg));
	fprintf(stderr,"%s: %s\n",msg, errmsg);
}

FFPacket::FFPacket()
{
	init();
}

FFPacket::~FFPacket()
{
	av_free_packet(&pkt);
}

void FFPacket::init()
{
	av_init_packet(&pkt);
	pkt.data = 0; pkt.size = 0;
}

FFrame::FFrame(FFStream *fst)
{
	this->fst = fst;
	frm = av_frame_alloc();
	init = fst->init_frame(frm);
}

FFrame::~FFrame()
{
	av_frame_free(&frm);
}

void FFrame::queue(int64_t pos)
{
	position = pos;
	fst->queue(this);
}

void FFrame::dequeue()
{
	fst->dequeue(this);
}

FFAudioHistory::FFAudioHistory()
{
	this->nch = 1;
	this->sz = 0x1000;
	bsz = sz * nch;
	bfr = new float[bsz];
	inp = outp = bfr;
	lmt = bfr + bsz;
}

FFAudioHistory::~FFAudioHistory()
{
	delete [] bfr;
}


void FFAudioHistory::reserve(long sz, int nch)
{
	long isz = inp - outp;
	sz += isz / nch;
	if( this->sz < sz || this->nch != nch ) {
		realloc(sz, nch);
		return;
	}
	if( isz > 0 )
		memmove(bfr, outp, isz*sizeof(*bfr));
	outp = bfr;
	inp = bfr + isz;
}

void FFAudioHistory::realloc(long sz, int nch)
{
	if( this->sz >= sz && this->nch == nch ) return;
	long isz = used() * nch;
	this->nch = nch;
	this->sz = sz;
	bsz = sz * nch;
	float *np = new float[bsz];
	if( isz > 0 ) copy(np, isz);
	inp = np + isz;
	outp = np;
	lmt = np + bsz;
	delete [] bfr;  bfr = np;
}

long FFAudioHistory::used()
{
	long len = inp>=outp ? inp-outp : inp-bfr + lmt-outp;
	return len / nch;
}
long FFAudioHistory::avail()
{
	float *in1 = inp+1;
	if( in1 >= lmt ) in1 = bfr;
	long len = outp >= in1 ? outp-in1 : outp-bfr + lmt-in1;
	return len / nch;
}
void FFAudioHistory::reset() // clear bfr
{
	inp = outp = bfr;
}

void FFAudioHistory::iseek(int64_t ofs)
{
	outp = inp - ofs*nch;
	if( outp < bfr ) outp += bsz;
}

float *FFAudioHistory::get_outp(int ofs)
{
	float *ret = outp;
	outp += ofs*nch;
	return ret;
}

int64_t FFAudioHistory::get_inp(int ofs)
{
	inp += ofs*nch;
	return (inp-bfr) / nch;
}

int FFAudioHistory::write(const float *fp, long len)
{
	long n = len * nch;
	while( n > 0 ) {
		int k = lmt - inp;
		if( k > n ) k = n;
		n -= k;
		while( --k >= 0 ) *inp++ = *fp++;
		if( inp >= lmt ) inp -= bsz;
	}
	return len;
}

int FFAudioHistory::copy(float *fp, long len)
{
	long n = len;
	while( n > 0 ) {
		int k = lmt - outp;
		if( k > n ) k = n;
		n -= k;
		while( --k >= 0 ) *fp++ = *outp++;
		if( outp >= lmt ) outp -= bsz;
	}
	return len;
}

int FFAudioHistory::zero(long len)
{
	long n = len * nch;
	while( n > 0 ) {
		int k = lmt - inp;
		if( k > n ) k = n;
		n -= k;
		while( --k >= 0 ) *inp++ = 0;
		if( inp >= lmt ) inp -= bsz;
	}
	return len;
}

int FFAudioHistory::read(double *dp, long len, int ch)
{
	long sz = used();
	if( !sz ) return 0;
	if( len > sz ) len = sz;
	long n = len;
	float *op = outp + ch;
	while( n > 0 ) {
		int k = (lmt - outp) / nch;
		if( k > n ) k = n;
		n -= k;
		while( --k >= 0 ) { *dp++ = *op;  op += nch; }
		if( op >= lmt ) op -= bsz;
	}
	return len;
}

// load linear buffer, no wrapping allowed, does not advance inp
int FFAudioHistory::write(const double *dp, long len, int ch)
{
	long osz = avail();
	if( !osz || !len ) return 0;
	if( len > osz ) len = osz;
	long n = len;
	float *ip = inp + ch;
	while( --n >= 0 ) { *ip = *dp++;  ip += nch; }
	return len;
}


FFStream::FFStream(FFMPEG *ffmpeg, AVStream *st, int idx)
{
	this->ffmpeg = ffmpeg;
	this->st = st;
	this->idx = idx;
	frm_lock = new Mutex("FFStream::frm_lock");
	fmt_ctx = 0;
	filter_graph = 0;
	buffersrc_ctx = 0;
	buffersink_ctx = 0;
	frm_count = 0;
	nudge = AV_NOPTS_VALUE;
	eof = 0;
	reading = writing = 0;
	need_packet = 1;
	flushed = 0;
	frame = fframe = 0;
}

FFStream::~FFStream()
{
	if( reading > 0 || writing > 0 ) avcodec_close(st->codec);
	if( fmt_ctx ) avformat_close_input(&fmt_ctx);
	while( frms.first ) frms.remove(frms.first);
	if( filter_graph ) avfilter_graph_free(&filter_graph);
	if( frame ) av_frame_free(&frame);
	if( fframe ) av_frame_free(&fframe);
	bsfilter.remove_all_objects();
	delete frm_lock;
}

void FFStream::ff_lock(const char *cp)
{
	FFMPEG::fflock.lock(cp);
}

void FFStream::ff_unlock()
{
	FFMPEG::fflock.unlock();
}

void FFStream::queue(FFrame *frm)
{
 	frm_lock->lock("FFStream::queue");
	frms.append(frm);
	++frm_count;
	frm_lock->unlock();
	ffmpeg->mux_lock->unlock();
}

void FFStream::dequeue(FFrame *frm)
{
	frm_lock->lock("FFStream::dequeue");
	--frm_count;
	frms.remove_pointer(frm);
	frm_lock->unlock();
}

int FFStream::encode_activate()
{
	if( writing < 0 )
		writing = ffmpeg->encode_activate();
	return writing;
}

int FFStream::decode_activate()
{
	if( reading < 0 && (reading=ffmpeg->decode_activate()) > 0 ) {
		ff_lock("FFStream::decode_activate");
		reading = 0;
		AVDictionary *copts = 0;
		av_dict_copy(&copts, ffmpeg->opts, 0);
		int ret = 0;
		// this should be avformat_copy_context(), but no copy avail
		ret = avformat_open_input(&fmt_ctx, ffmpeg->fmt_ctx->filename, NULL, &copts);
		if( ret >= 0 ) {
			ret = avformat_find_stream_info(fmt_ctx, 0);
			st = fmt_ctx->streams[idx];
		}
		if( ret >= 0 ) {
			AVCodecID codec_id = st->codec->codec_id;
			AVCodec *decoder = avcodec_find_decoder(codec_id);
			ret = avcodec_open2(st->codec, decoder, &copts);
			if( ret >= 0 )
				reading = 1;
			else
				fprintf(stderr, "FFStream::decode_activate: open decoder failed\n");
		}
		else
			fprintf(stderr, "FFStream::decode_activate: can't clone input file\n");
		av_dict_free(&copts);
		ff_unlock();
	}
	return reading;
}

int FFStream::read_packet()
{
	av_packet_unref(ipkt);
	int ret = av_read_frame(fmt_ctx, ipkt);
	if( ret >= 0 ) return 1;
	st_eof(1);
	if( ret == AVERROR_EOF ) return 0;
	fprintf(stderr, "FFStream::read_packet: av_read_frame failed\n");
	flushed = 1;
	return -1;
}

int FFStream::decode(AVFrame *frame)
{
	int ret = 0;
	int retries = 100;
	int got_frame = 0;

	while( ret >= 0 && !flushed && --retries >= 0 && !got_frame ) {
		if( need_packet ) {
			need_packet = 0;
			ret = read_packet();
			if( ret < 0 ) break;
			if( !ret ) ipkt->stream_index = st->index;
		}
		if( ipkt->stream_index == st->index ) {
			while( ipkt->size > 0 && !got_frame ) {
				ret = decode_frame(frame, got_frame);
				if( ret < 0 ) break;
				ipkt->data += ret;
				ipkt->size -= ret;
			}
			retries = 100;
		}
		if( !got_frame ) {
			need_packet = 1;
			flushed = st_eof();
		}
	}

	if( retries < 0 )
		fprintf(stderr, "FFStream::decode: Retry limit\n");
	if( ret >= 0 )
		ret = got_frame;
	else
		fprintf(stderr, "FFStream::decode: failed\n");

	return ret;
}

int FFStream::load_filter(AVFrame *frame)
{
	int ret = av_buffersrc_add_frame_flags(buffersrc_ctx,
			frame, AV_BUFFERSRC_FLAG_KEEP_REF);
	if( ret < 0 ) {
		av_frame_unref(frame);
		fprintf(stderr, "FFStream::load_filter: av_buffersrc_add_frame_flags failed\n");
	}
	return ret;
}

int FFStream::read_filter(AVFrame *frame)
{
	int ret = av_buffersink_get_frame(buffersink_ctx, frame);
	if( ret < 0 ) {
		if( ret == AVERROR(EAGAIN) ) return 0;
		if( ret == AVERROR_EOF ) { st_eof(1); return -1; }
		fprintf(stderr, "FFStream::read_filter: av_buffersink_get_frame failed\n");
		return ret;
	}
	return 1;
}

int FFStream::read_frame(AVFrame *frame)
{
	if( !filter_graph || !buffersrc_ctx || !buffersink_ctx )
		return decode(frame);
	if( !fframe && !(fframe=av_frame_alloc()) ) {
		fprintf(stderr, "FFStream::read_frame: av_frame_alloc failed\n");
		return -1;
	}
	int ret = -1;
	while( !flushed && !(ret=read_filter(frame)) ) {
		if( (ret=decode(fframe)) < 0 ) break;
		if( ret > 0 && (ret=load_filter(fframe)) < 0 ) break;
	}
	return ret;
}

FFAudioStream::FFAudioStream(FFMPEG *ffmpeg, AVStream *strm, int idx)
 : FFStream(ffmpeg, strm, idx)
{
	channel0 = channels = 0;
	sample_rate = 0;
	mbsz = 0;
	seek_pos = curr_pos = 0;
	length = 0;
	resample_context = 0;

	aud_bfr_sz = 0;
	aud_bfr = 0;
}

FFAudioStream::~FFAudioStream()
{
	if( resample_context ) swr_free(&resample_context);
	delete [] aud_bfr;
}

int FFAudioStream::load_history(float *&bfr, int len)
{
	if( resample_context ) {
		if( len > aud_bfr_sz ) {	
			delete [] aud_bfr;
			aud_bfr = 0;
		}
		if( !aud_bfr ) {
			aud_bfr_sz = len;
			aud_bfr = new float[aud_bfr_sz*channels];
		}
		int ret = swr_convert(resample_context,
			(uint8_t**)&aud_bfr, aud_bfr_sz,
			(const uint8_t**)&bfr, len);
		if( ret < 0 ) {
			fprintf(stderr, "FFAudioStream::load_history: swr_convert failed\n");
			return -1;
		}
		bfr = aud_bfr;
		len = ret;
	}
	append_history(bfr, len);
	return len;
}

int FFAudioStream::decode_frame(AVFrame *frame, int &got_frame)
{
	int ret = avcodec_decode_audio4(st->codec, frame, &got_frame, ipkt);
	if( ret < 0 ) {
		fprintf(stderr, "FFAudioStream::decode_frame: Could not read audio frame\n");
		return -1;
	}
	return ret;
}

int FFAudioStream::encode_activate()
{
	if( writing >= 0 ) return writing;
	AVCodecContext *ctx = st->codec;
	frame_sz = ctx->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
		10000 : ctx->frame_size;
	return FFStream::encode_activate();
}

int FFAudioStream::nb_samples()
{
	AVCodecContext *ctx = st->codec;
	return ctx->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
                10000 : ctx->frame_size;
}

void FFAudioStream::alloc_history(int len)
{
	history.realloc(len+1, channels);
}

void FFAudioStream::reserve_history(int len)
{
	history.reserve(len+1, st->codec->channels);
}

void FFAudioStream::append_history(const float *fp, int len)
{
	// biggest user bfr since seek + length this frame
	int hsz = mbsz + len;
	alloc_history(hsz);
	history.write(fp, len);
}

int64_t FFAudioStream::load_buffer(double ** const sp, int len)
{
	reserve_history(len);
	int nch = st->codec->channels;
	for( int ch=0; ch<nch; ++ch )
		history.write(sp[ch], len, ch);
	return history.get_inp(len);
}

void FFAudioStream::zero_history(int len)
{
	history.zero(len);
}

float* FFAudioStream::get_history(int len)
{
	return history.get_outp(len);
}

int FFAudioStream::in_history(int64_t pos)
{
	if( pos > curr_pos ) return 0;
	int64_t len = curr_pos - seek_pos;
	if( len > history.sz ) len = history.sz;
	if( pos < curr_pos - len ) return 0;
	return 1;
}


int FFAudioStream::init_frame(AVFrame *frame)
{
	AVCodecContext *ctx = st->codec;
	frame->nb_samples = frame_sz;
	frame->format = ctx->sample_fmt;
	frame->channel_layout = ctx->channel_layout;
	frame->sample_rate = ctx->sample_rate;
	int ret = av_frame_get_buffer(frame, 0);
	if (ret < 0)
		fprintf(stderr, "FFAudioStream::init_frame: av_frame_get_buffer failed\n");
	return ret;
}

int FFAudioStream::load(int64_t pos, int len)
{
	if( audio_seek(pos) < 0 ) return -1;
	if( mbsz < len ) mbsz = len;
	int ret = 0;
	int64_t end_pos = pos + len;
	if( !frame && !(frame=av_frame_alloc()) ) {
		fprintf(stderr, "FFAudioStream::load: av_frame_alloc failed\n");
		return -1;
	}
	for( int i=0; ret>=0 && !flushed && curr_pos<end_pos && i<1000; ++i ) {
		ret = read_frame(frame);
		if( ret > 0 ) {
			load_history((float *&)frame->extended_data[0], frame->nb_samples);
			curr_pos += frame->nb_samples;
		}
	}
	if( flushed && end_pos > curr_pos ) {
		zero_history(end_pos - curr_pos);
		curr_pos = end_pos;
	}
	return curr_pos - pos;
}

int FFAudioStream::audio_seek(int64_t pos)
{
	if( decode_activate() < 0 ) return -1;
	if( in_history(pos) ) {
		history.iseek(curr_pos - pos);
		return 0;
	}
	if( pos == curr_pos ) return 0;
	double secs = (double)pos / sample_rate;
	int64_t tstmp = secs * st->time_base.den / st->time_base.num;
	if( nudge != AV_NOPTS_VALUE ) tstmp += nudge;
	avformat_seek_file(fmt_ctx, st->index, -INT64_MAX, tstmp, INT64_MAX, 0);
	seek_pos = curr_pos = pos;
	mbsz = 0;
	history.reset();
	st_eof(0);
	return 1;
}

int FFAudioStream::encode(double **samples, int len)
{
	if( encode_activate() <= 0 ) return -1;
	ffmpeg->flow_ctl();
	int ret = 0;
	int64_t count = load_buffer(samples, len);
	FFrame *frm = 0;

	while( ret >= 0 && count >= frame_sz ) {
		frm = new FFrame(this);
		if( (ret=frm->initted()) < 0 ) break;
		AVFrame *frame = *frm;
		float *bfrp = get_history(frame_sz);
		ret =  swr_convert(resample_context,
			(uint8_t **)frame->extended_data, frame_sz,
			(const uint8_t **)&bfrp, frame_sz);
		if( ret < 0 ) {
			fprintf(stderr, "FFAudioStream::encode: swr_convert failed\n");
			break;
		}
		frm->queue(curr_pos);
		frm = 0;
		curr_pos += frame_sz;
		count -= frame_sz;
	}

	delete frm;
	return ret >= 0 ? 0 : 1;
}

FFVideoStream::FFVideoStream(FFMPEG *ffmpeg, AVStream *strm, int idx)
 : FFStream(ffmpeg, strm, idx)
{
	width = height = 0;
	frame_rate = 0;
	aspect_ratio = 0;
	seek_pos = curr_pos = 0;
	length = 0;
	convert_ctx = 0;
}

FFVideoStream::~FFVideoStream()
{
	if( convert_ctx ) sws_freeContext(convert_ctx);
}

int FFVideoStream::decode_frame(AVFrame *frame, int &got_frame)
{
	int ret = avcodec_decode_video2(st->codec, frame, &got_frame, ipkt);
	if( ret < 0 ) {
		fprintf(stderr, "FFVideoStream::decode_frame: Could not read video frame\n");
		return -1;
	}
	if( got_frame )
		++curr_pos;
	return ret;
}

int FFVideoStream::load(VFrame *vframe, int64_t pos)
{
	if( video_seek(pos) < 0 ) return -1;
	if( !frame && !(frame=av_frame_alloc()) ) {
		fprintf(stderr, "FFVideoStream::load: av_frame_alloc failed\n");
		return -1;
	}
	int ret = 0;
	for( int i=0; ret>=0 && !flushed && curr_pos<=pos && i<1000; ++i ) {
		ret = read_frame(frame);
	}
	if( ret > 0 ) {
		AVCodecContext *ctx = st->codec;
		ret = convert_cmodel(vframe, (AVPicture *)frame,
			ctx->pix_fmt, ctx->width, ctx->height);
	}
	ret = ret > 0 ? 1 : ret < 0 ? -1 : 0;
	return ret;
}

int FFVideoStream::video_seek(int64_t pos)
{
	if( decode_activate() < 0 ) return -1;
// if close enough, just read up to current
//   3*gop_size seems excessive, but less causes tears
	int gop = 3*st->codec->gop_size;
	if( gop < 4 ) gop = 4;
	if( gop > 64 ) gop = 64;
	if( pos >= curr_pos && pos <= curr_pos + gop ) return 0;
// back up a few frames to read up to current to help repair damages
	if( (pos-=gop) < 0 ) pos = 0;
	double secs = (double)pos / frame_rate;
	int64_t tstmp = secs * st->time_base.den / st->time_base.num;
	if( nudge != AV_NOPTS_VALUE ) tstmp += nudge;
	avformat_seek_file(fmt_ctx, st->index, -INT64_MAX, tstmp, INT64_MAX, 0);
	seek_pos = curr_pos = pos;
	st_eof(0);
	return 1;
}

int FFVideoStream::init_frame(AVFrame *picture)
{
	AVCodecContext *ctx = st->codec;
	picture->format = ctx->pix_fmt;
	picture->width  = ctx->width;
	picture->height = ctx->height;
	int ret = av_frame_get_buffer(picture, 32);
	return ret;
}

int FFVideoStream::encode(VFrame *vframe)
{
	if( encode_activate() <= 0 ) return -1;
	ffmpeg->flow_ctl();
	FFrame *picture = new FFrame(this);
	int ret = picture->initted();
	if( ret >= 0 ) {
		AVFrame *frame = *picture;
		frame->pts = curr_pos;
		AVCodecContext *ctx = st->codec;
		ret = convert_pixfmt(vframe, (AVPicture*)frame,
			ctx->pix_fmt, ctx->width, ctx->height);
	}
	if( ret >= 0 ) {
		picture->queue(curr_pos);
		++curr_pos;
	}
	else {
		fprintf(stderr, "FFVideoStream::encode: encode failed\n");
		delete picture;
	}
	return ret >= 0 ? 0 : 1;
}


PixelFormat FFVideoStream::color_model_to_pix_fmt(int color_model)
{
	switch( color_model ) { 
	case BC_YUV422:		return AV_PIX_FMT_YUYV422;
	case BC_RGB888:		return AV_PIX_FMT_RGB24;
	case BC_RGBA8888:	return AV_PIX_FMT_RGBA;
	case BC_BGR8888:	return AV_PIX_FMT_BGR0;
	case BC_BGR888:		return AV_PIX_FMT_BGR24;
	case BC_YUV420P:	return AV_PIX_FMT_YUV420P;
	case BC_YUV422P:	return AV_PIX_FMT_YUV422P;
	case BC_YUV444P:	return AV_PIX_FMT_YUV444P;
	case BC_YUV411P:	return AV_PIX_FMT_YUV411P;
	case BC_RGB565:		return AV_PIX_FMT_RGB565;
	case BC_RGB161616:      return AV_PIX_FMT_RGB48LE;
	case BC_RGBA16161616:   return AV_PIX_FMT_RGBA64LE;
	default: break;
	}

	return AV_PIX_FMT_NB;
}

int FFVideoStream::pix_fmt_to_color_model(PixelFormat pix_fmt)
{
	switch (pix_fmt) { 
	case AV_PIX_FMT_YUYV422:	return BC_YUV422;
	case AV_PIX_FMT_RGB24:		return BC_RGB888;
	case AV_PIX_FMT_RGBA:		return BC_RGBA8888;
	case AV_PIX_FMT_BGR0:		return BC_BGR8888;
	case AV_PIX_FMT_BGR24:		return BC_BGR888;
	case AV_PIX_FMT_YUV420P:	return BC_YUV420P;
	case AV_PIX_FMT_YUV422P:	return BC_YUV422P;
	case AV_PIX_FMT_YUV444P:	return BC_YUV444P;
	case AV_PIX_FMT_YUV411P:	return BC_YUV411P;
	case AV_PIX_FMT_RGB565:		return BC_RGB565;
	case AV_PIX_FMT_RGB48LE:	return BC_RGB161616;
	case AV_PIX_FMT_RGBA64LE:	return BC_RGBA16161616;
	default: break;
	}

	return BC_TRANSPARENCY;
}

int FFVideoStream::convert_picture_vframe(VFrame *frame,
		AVPicture *ip, PixelFormat ifmt, int iw, int ih)
{
	AVPicture opic;
	int cmodel = frame->get_color_model();
	PixelFormat ofmt = color_model_to_pix_fmt(cmodel);
	if( ofmt == AV_PIX_FMT_NB ) return -1;
	int size = avpicture_fill(&opic, frame->get_data(), ofmt, 
				  frame->get_w(), frame->get_h());
	if( size < 0 ) return -1;

	// transfer line sizes must match also
	int planar = BC_CModels::is_planar(cmodel);
	int packed_width = !planar ? frame->get_bytes_per_line() :
		 BC_CModels::calculate_pixelsize(cmodel) * frame->get_w();
	if( packed_width != opic.linesize[0] )  return -1;

	if( planar ) {
		// override avpicture_fill() for planar types
		opic.data[0] = frame->get_y();
		opic.data[1] = frame->get_u();
		opic.data[2] = frame->get_v();
	}

	convert_ctx = sws_getCachedContext(convert_ctx, iw, ih, ifmt,
		frame->get_w(), frame->get_h(), ofmt, SWS_BICUBIC, NULL, NULL, NULL);
	if( !convert_ctx ) {
		fprintf(stderr, "FFVideoStream::convert_picture_frame:"
				" sws_getCachedContext() failed\n");
		return 1;
	}
	if( sws_scale(convert_ctx, ip->data, ip->linesize, 0, ih,
	    opic.data, opic.linesize) < 0 ) {
		fprintf(stderr, "FFVideoStream::convert_picture_frame: sws_scale() failed\n");
		return 1;
	}
	return 0;
}

int FFVideoStream::convert_cmodel(VFrame *frame,
		 AVPicture *ip, PixelFormat ifmt, int iw, int ih)
{
	// try direct transfer
	if( !convert_picture_vframe(frame, ip, ifmt, iw, ih) ) return 1;
	// use indirect transfer
	const AVPixFmtDescriptor *desc = av_pix_fmt_desc_get(ifmt);
	int max_bits = 0;
	for( int i = 0; i <desc->nb_components; ++i ) {
		int bits = desc->comp[i].depth_minus1 + 1;
		if( bits > max_bits ) max_bits = bits;
	}
// from libavcodec/pixdesc.c
#define pixdesc_has_alpha(pixdesc) ((pixdesc)->nb_components == 2 || \
 (pixdesc)->nb_components == 4 || (pixdesc)->flags & AV_PIX_FMT_FLAG_PAL)
	int icolor_model = pixdesc_has_alpha(desc) ?
		(max_bits > 8 ? BC_RGBA16161616 : BC_RGBA8888) :
		(max_bits > 8 ? BC_RGB161616 : BC_RGB888) ;
	VFrame vframe(iw, ih, icolor_model);
	if( convert_picture_vframe(&vframe, ip, ifmt, iw, ih) ) return -1;
	frame->transfer_from(&vframe);
	return 1;
}

int FFVideoStream::convert_vframe_picture(VFrame *frame,
		AVPicture *op, PixelFormat ofmt, int ow, int oh)
{
	AVPicture opic;
	int cmodel = frame->get_color_model();
	PixelFormat ifmt = color_model_to_pix_fmt(cmodel);
	if( ifmt == AV_PIX_FMT_NB ) return -1;
	int size = avpicture_fill(&opic, frame->get_data(), ifmt, 
				  frame->get_w(), frame->get_h());
	if( size < 0 ) return -1;

	// transfer line sizes must match also
	int planar = BC_CModels::is_planar(cmodel);
	int packed_width = !planar ? frame->get_bytes_per_line() :
		 BC_CModels::calculate_pixelsize(cmodel) * frame->get_w();
	if( packed_width != opic.linesize[0] )  return -1;

	if( planar ) {
		// override avpicture_fill() for planar types
		opic.data[0] = frame->get_y();
		opic.data[1] = frame->get_u();
		opic.data[2] = frame->get_v();
	}

	convert_ctx = sws_getCachedContext(convert_ctx, frame->get_w(), frame->get_h(), ifmt,
		ow, oh, ofmt, SWS_BICUBIC, NULL, NULL, NULL);
	if( !convert_ctx ) {
		fprintf(stderr, "FFVideoStream::convert_frame_picture:"
				" sws_getCachedContext() failed\n");
		return 1;
	}
	if( sws_scale(convert_ctx, opic.data, opic.linesize, 0, frame->get_h(),
			op->data, op->linesize) < 0 ) {
		fprintf(stderr, "FFVideoStream::convert_frame_picture: sws_scale() failed\n");
		return 1;
	}
	return 0;
}

int FFVideoStream::convert_pixfmt(VFrame *frame,
		 AVPicture *op, PixelFormat ofmt, int ow, int oh)
{
	// try direct transfer
	if( !convert_vframe_picture(frame, op, ofmt, ow, oh) ) return 0;
	// use indirect transfer
	int colormodel = frame->get_color_model();
	int bits = BC_CModels::calculate_pixelsize(colormodel) * 8;
	bits /= BC_CModels::components(colormodel);
	int icolor_model =  BC_CModels::has_alpha(colormodel) ?
		(bits > 8 ? BC_RGBA16161616 : BC_RGBA8888) :
		(bits > 8 ? BC_RGB161616: BC_RGB888) ;
	VFrame vframe(frame->get_w(), frame->get_h(), icolor_model);
	vframe.transfer_from(frame);
	if( convert_vframe_picture(&vframe, op, ofmt, ow, oh) ) return 1;
	return 0;
}


FFMPEG::FFMPEG(FileBase *file_base)
{
	fmt_ctx = 0;
	this->file_base = file_base;
	memset(file_format,0,sizeof(file_format));
	mux_lock = new Condition(0,"FFMPEG::mux_lock",0);
	flow_lock = new Condition(1,"FFStream::flow_lock",0);
	done = -1;
	flow = 1;
	decoding = encoding = 0;
	has_audio = has_video = 0;
	opts = 0;
	opt_duration = -1;
	opt_video_filter = 0;
	opt_audio_filter = 0;
	char option_path[BCTEXTLEN];
	set_option_path(option_path, "%s", "ffmpeg.opts");
	read_options(option_path, opts);
}

FFMPEG::~FFMPEG()
{
	ff_lock("FFMPEG::~FFMPEG()");
	close_encoder();
	ffaudio.remove_all_objects();
	ffvideo.remove_all_objects();
	if( encoding ) avformat_free_context(fmt_ctx);
	ff_unlock();
	delete flow_lock;
	delete mux_lock;
	av_dict_free(&opts);
	delete opt_video_filter;
	delete opt_audio_filter;
}

int FFMPEG::check_sample_rate(AVCodec *codec, int sample_rate)
{
	const int *p = codec->supported_samplerates;
	if( !p ) return sample_rate;
	while( *p != 0 ) {
		if( *p == sample_rate ) return *p;
		++p;
	}
	return 0;
}

static inline AVRational std_frame_rate(int i)
{
	static const int m1 = 1001*12, m2 = 1000*12;
	static const int freqs[] = {
		40*m1, 48*m1, 50*m1, 60*m1, 80*m1,120*m1, 240*m1,
		24*m2, 30*m2, 60*m2, 12*m2, 15*m2, 48*m2, 0,
	};
	int freq = i<30*12 ? (i+1)*1001 : freqs[i-30*12];
	return (AVRational) { freq, 1001*12 };
}

AVRational FFMPEG::check_frame_rate(AVCodec *codec, double frame_rate)
{
	const AVRational *p = codec->supported_framerates;
	AVRational rate, best_rate = (AVRational) { 0, 0 };
	double max_err = 1.;  int i = 0;
	while( ((p ? (rate=*p++) : (rate=std_frame_rate(i++))), rate.num) != 0 ) {
		double framerate = (double) rate.num / rate.den;
		double err = fabs(frame_rate/framerate - 1.);
		if( err >= max_err ) continue;
		max_err = err;
		best_rate = rate;
	}
	return max_err < 0.0001 ? best_rate : (AVRational) { 0, 0 };
}

AVRational FFMPEG::to_sample_aspect_ratio(double aspect_ratio)
{
	int height = 1000000, width = height * aspect_ratio;
	float w, h;
	MWindow::create_aspect_ratio(w, h, width, height);
	return (AVRational){(int)w, (int)h};
}

AVRational FFMPEG::to_time_base(int sample_rate)
{
	return (AVRational){1, sample_rate};
}

extern void get_exe_path(char *result); // from main.C

void FFMPEG::set_option_path(char *path, const char *fmt, ...)
{
	get_exe_path(path);
	strcat(path, "/ffmpeg/");
	path += strlen(path);
	va_list ap;
	va_start(ap, fmt);
	vsprintf(path, fmt, ap);
	va_end(ap);
}

void FFMPEG::get_option_path(char *path, const char *type, const char *spec)
{
	if( *spec == '/' )
		strcpy(path, spec);
	else
		set_option_path(path, "%s/%s", type, spec);
}

int FFMPEG::check_option(const char *path, char *spec)
{
	char option_path[BCTEXTLEN], line[BCTEXTLEN];
	char format[BCSTRLEN], codec[BCSTRLEN];
	get_option_path(option_path, path, spec);
	FILE *fp = fopen(option_path,"r");
	if( !fp ) return 1;
	int ret = 0;
	if( !fgets(line, sizeof(line), fp) ) ret = 1;
	if( !ret ) {
		line[sizeof(line)-1] = 0;
		ret = scan_option_line(line, format, codec);
	}
	if( !ret ) {
		if( !file_format[0] ) strcpy(file_format, format);
		else if( strcmp(file_format, format) ) ret = 1;
	}
	fclose(fp);
	return ret;
}

const char *FFMPEG::get_file_format()
{
	file_format[0] = 0;
	int ret = 0;
	Asset *asset = file_base->asset;
	if( !ret && asset->audio_data )
		ret = check_option("audio", asset->acodec);
	if( !ret && asset->video_data )
		ret = check_option("video", asset->vcodec);
	if( !ret && !file_format[0] ) ret = 1;
	return !ret ? file_format : 0;
}

int FFMPEG::scan_option_line(char *cp, char *tag, char *val)
{
	while( *cp == ' ' || *cp == '\t' ) ++cp;
	char *bp = cp;
	while( *cp && *cp != ' ' && *cp != '\t' && *cp != '=' ) ++cp;
	int len = cp - bp;
	if( !len || len > BCSTRLEN-1 ) return 1;
	while( bp < cp ) *tag++ = *bp++;
	*tag = 0;
	while( *cp == ' ' || *cp == '\t' ) ++cp;
	if( *cp == '=' ) ++cp;
	while( *cp == ' ' || *cp == '\t' ) ++cp;
	bp = cp;
	while( *cp && *cp != '\n' ) ++cp;
	len = cp - bp;
	if( len > BCTEXTLEN-1 ) return 1;
	while( bp < cp ) *val++ = *bp++;
	*val = 0;
	return 0;
}

int FFMPEG::read_options(const char *options, char *format, char *codec,
		char *bsfilter, char *bsargs, AVDictionary *&opts)
{
	FILE *fp = fopen(options,"r");
	if( !fp ) {
		fprintf(stderr, "FFMPEG::read_options: options open failed %s\n",options);
		return 1;
	}
	int ret = read_options(fp, options, format, codec, opts);
	char *cp = codec;
	while( *cp && *cp != '|' ) ++cp;
	if( *cp == '|' && !scan_option_line(cp+1, bsfilter, bsargs) ) {
		do { *cp-- = 0; } while( cp>=codec && (*cp==' ' || *cp == '\t' ) );
	}
	else
		bsfilter[0] = bsargs[0] = 0;
	fclose(fp);
	return ret;
}

int FFMPEG::read_options(FILE *fp, const char *options,
		 char *format, char *codec, AVDictionary *&opts)
{
	char line[BCTEXTLEN];
	if( !fgets(line, sizeof(line), fp) ) {
		fprintf(stderr, "FFMPEG::read_options:"
			" options file empty %s\n",options);
		return 1;
	}
	line[sizeof(line)-1] = 0;
	if( scan_option_line(line, format, codec) ) {
		fprintf(stderr, "FFMPEG::read_options:"
			" err: format/codec not found %s\n", options);
		return 1;
	}
	return read_options(fp, options, opts, 1);
}

int FFMPEG::read_options(const char *options, AVDictionary *&opts)
{
	FILE *fp = fopen(options,"r");
	if( !fp ) return 1;
	int ret = read_options(fp, options, opts);
	fclose(fp);
	return ret;
}

int FFMPEG::read_options(FILE *fp, const char *options, AVDictionary *&opts, int no)
{
	int ret = 0;
	char line[BCTEXTLEN];
	while( !ret && fgets(line, sizeof(line), fp) ) {
		line[sizeof(line)-1] = 0;
		++no;
		if( line[0] == '#' ) continue;
		char key[BCSTRLEN], val[BCTEXTLEN];
		if( scan_option_line(line, key, val) ) {
			fprintf(stderr, "FFMPEG::read_options:"
				" err reading %s: line %d\n", options, no);
			ret = 1;
		}
		if( !ret ) {
			if( !strcmp(key, "duration") )
				opt_duration = strtod(val, 0);
			if( !strcmp(key, "video_filter") )
				opt_video_filter = cstrdup(val);
			if( !strcmp(key, "audio_filter") )
				opt_audio_filter = cstrdup(val);
			else if( !strcmp(key, "loglevel") )
				set_loglevel(val);
			else
				av_dict_set(&opts, key, val, 0);
		}
	}
	return ret;
}

int FFMPEG::load_options(const char *options, AVDictionary *&opts)
{
	char option_path[BCTEXTLEN];
	set_option_path(option_path, "%s", options);
	return read_options(option_path, opts);
}

void FFMPEG::set_loglevel(const char *ap)
{
	if( !ap || !*ap ) return;
	const struct {
		const char *name;
		int level;
	} log_levels[] = {
		{ "quiet"  , AV_LOG_QUIET   },
		{ "panic"  , AV_LOG_PANIC   },
		{ "fatal"  , AV_LOG_FATAL   },
		{ "error"  , AV_LOG_ERROR   },
		{ "warning", AV_LOG_WARNING },
		{ "info"   , AV_LOG_INFO    },
		{ "verbose", AV_LOG_VERBOSE },
		{ "debug"  , AV_LOG_DEBUG   },
	};
	for( int i=0; i<(int)(sizeof(log_levels)/sizeof(log_levels[0])); ++i ) {
		if( !strcmp(log_levels[i].name, ap) ) {
			av_log_set_level(log_levels[i].level);
			return;
		}
	}
	av_log_set_level(atoi(ap));
}

double FFMPEG::to_secs(int64_t time, AVRational time_base)
{
	double base_time = time == AV_NOPTS_VALUE ? 0 :
		av_rescale_q(time, time_base, AV_TIME_BASE_Q);
	return base_time / AV_TIME_BASE; 
}

int FFMPEG::info(char *text, int len)
{
	if( len <= 0 ) return 0;
#define report(s...) do { int n = snprintf(cp,len,s); cp += n;  len -= n; } while(0)
	char *cp = text;
	for( int i=0; i<(int)fmt_ctx->nb_streams; ++i ) {
		AVStream *st = fmt_ctx->streams[i];
		AVCodecContext *avctx = st->codec;
		report("stream %d,  id 0x%06x:\n", i, avctx->codec_id);
		const AVCodecDescriptor *desc = avcodec_descriptor_get(avctx->codec_id);
		if( avctx->codec_type == AVMEDIA_TYPE_VIDEO ) {
			AVRational framerate = av_guess_frame_rate(fmt_ctx, st, 0);
			double frame_rate = !framerate.den ? 0 : (double)framerate.num / framerate.den;
			report("  video %s",desc ? desc->name : " (unkn)");
			report(" %dx%d %5.2f", avctx->width, avctx->height, frame_rate);
			const char *pfn = av_get_pix_fmt_name(avctx->pix_fmt);
			report(" pix %s\n", pfn ? pfn : "(unkn)");
			double secs = to_secs(st->duration, st->time_base);
			int64_t length = secs * frame_rate + 0.5;
			report("    %jd frms %0.2f secs", length, secs);
			int hrs = secs/3600;  secs -= hrs*3600;
			int mins = secs/60;  secs -= mins*60;
			report("  %d:%02d:%05.2f\n", hrs, mins, secs);

		}
		else if( avctx->codec_type == AVMEDIA_TYPE_AUDIO ) {
			int sample_rate = avctx->sample_rate;
			const char *fmt = av_get_sample_fmt_name(avctx->sample_fmt);
			report("  audio %s",desc ? desc->name : " (unkn)");
			report(" %dch %s %d",avctx->channels, fmt, sample_rate);
			int sample_bits = av_get_bits_per_sample(avctx->codec_id);
			report(" %dbits\n", sample_bits);
			double secs = to_secs(st->duration, st->time_base);
			int64_t length = secs * sample_rate + 0.5;
			report("    %jd smpl %0.2f secs", length, secs);
			int hrs = secs/3600;  secs -= hrs*3600;
			int mins = secs/60;  secs -= mins*60;
			report("  %d:%02d:%05.2f\n", hrs, mins, secs);
		}
		else
			report("  codec_type unknown\n");
	}
	report("\n");
	for( int i=0; i<(int)fmt_ctx->nb_programs; ++i ) {
		report("program %d", i+1);
		AVProgram *pgrm = fmt_ctx->programs[i];
		for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j )
			report(", %d", pgrm->stream_index[j]);
		report("\n");
	}
	report("\n");
	AVDictionaryEntry *tag = 0;
	while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
		report("%s=%s\n", tag->key, tag->value);

	if( !len ) --cp;
	*cp = 0;
	return cp - text;
#undef report
}


int FFMPEG::init_decoder(const char *filename)
{
	ff_lock("FFMPEG::init_decoder");
	av_register_all();
	char file_opts[BCTEXTLEN];
	char *bp = strrchr(strcpy(file_opts, filename), '/');
	char *sp = strrchr(!bp ? file_opts : bp, '.');
	FILE *fp = 0;
	if( sp ) {
		strcpy(sp, ".opts");
		fp = fopen(file_opts, "r");
	}
	if( fp ) {
		read_options(fp, file_opts, opts, 0);
		fclose(fp);
	}
	else
		load_options("decode.opts", opts);
	AVDictionary *fopts = 0;
	av_dict_copy(&fopts, opts, 0);
	int ret = avformat_open_input(&fmt_ctx, filename, NULL, &fopts);
	av_dict_free(&fopts);
	if( ret >= 0 )
		ret = avformat_find_stream_info(fmt_ctx, NULL);
	if( !ret ) {
		decoding = -1;
	}
	ff_unlock();
	return !ret ? 0 : 1;
}

int FFMPEG::open_decoder()
{
	struct stat st;
	if( stat(fmt_ctx->filename, &st) < 0 ) {
		fprintf(stderr,"FFMPEG::open_decoder: can't stat file: %s\n",
			fmt_ctx->filename);
		return 1;
	}

	int64_t file_bits = 8 * st.st_size;
	if( !fmt_ctx->bit_rate && opt_duration > 0 )
		fmt_ctx->bit_rate = file_bits / opt_duration;

	int estimated = 0;
	if( fmt_ctx->bit_rate > 0 ) {
		for( int i=0; i<(int)fmt_ctx->nb_streams; ++i ) {
			AVStream *st = fmt_ctx->streams[i];
			if( st->duration != AV_NOPTS_VALUE ) continue;
			if( st->time_base.num > INT64_MAX / fmt_ctx->bit_rate ) continue;
			st->duration = av_rescale(file_bits, st->time_base.den,
				fmt_ctx->bit_rate * (int64_t) st->time_base.num);
			estimated = 1;
		}
	}
	if( estimated )
		printf("FFMPEG::open_decoder: some stream times estimated\n");

	ff_lock("FFMPEG::open_decoder");
	int bad_time = 0;
	for( int i=0; i<(int)fmt_ctx->nb_streams; ++i ) {
		AVStream *st = fmt_ctx->streams[i];
		if( st->duration == AV_NOPTS_VALUE ) bad_time = 1;
		AVCodecContext *avctx = st->codec;
		if( avctx->codec_type == AVMEDIA_TYPE_VIDEO ) {
			has_video = 1;
			FFVideoStream *vid = new FFVideoStream(this, st, i);
			int vidx = ffvideo.size();
			vstrm_index.append(ffidx(vidx, 0));
			ffvideo.append(vid);
			vid->width = avctx->width;
			vid->height = avctx->height;
			AVRational framerate = av_guess_frame_rate(fmt_ctx, st, 0);
			vid->frame_rate = !framerate.den ? 0 : (double)framerate.num / framerate.den;
			double secs = to_secs(st->duration, st->time_base);
			vid->length = secs * vid->frame_rate;
			vid->aspect_ratio = (double)st->sample_aspect_ratio.num / st->sample_aspect_ratio.den;
			vid->nudge = st->start_time;
			vid->reading = -1;
			if( opt_video_filter )
				vid->create_filter(opt_video_filter, avctx,avctx);
		}
		else if( avctx->codec_type == AVMEDIA_TYPE_AUDIO ) {
			has_audio = 1;
			FFAudioStream *aud = new FFAudioStream(this, st, i);
			int aidx = ffaudio.size();
			ffaudio.append(aud);
			aud->channel0 = astrm_index.size();
			aud->channels = avctx->channels;
			for( int ch=0; ch<aud->channels; ++ch )
				astrm_index.append(ffidx(aidx, ch));
			aud->sample_rate = avctx->sample_rate;
			double secs = to_secs(st->duration, st->time_base);
			aud->length = secs * aud->sample_rate;
			if( avctx->sample_fmt != AV_SAMPLE_FMT_FLT ) {
				uint64_t layout = av_get_default_channel_layout(avctx->channels);
				if( !layout ) layout = ((uint64_t)1<<aud->channels) - 1;
				aud->resample_context = swr_alloc_set_opts(NULL,
					layout, AV_SAMPLE_FMT_FLT, avctx->sample_rate,
					layout, avctx->sample_fmt, avctx->sample_rate,
					0, NULL);
				swr_init(aud->resample_context);
			}
			aud->nudge = st->start_time;
			aud->reading = -1;
			if( opt_audio_filter )
				aud->create_filter(opt_audio_filter, avctx,avctx);
		}
	}
	if( bad_time )
		printf("FFMPEG::open_decoder: some stream have bad times\n");
	ff_unlock();
	return 0;
}


int FFMPEG::init_encoder(const char *filename)
{
	const char *format = get_file_format();
	if( !format ) {
		fprintf(stderr, "FFMPEG::init_encoder: invalid file format for %s\n", filename);
		return 1;
	}
	int ret = 0;
	ff_lock("FFMPEG::init_encoder");
	av_register_all();
	avformat_alloc_output_context2(&fmt_ctx, 0, format, filename);
	if( !fmt_ctx ) {
		fprintf(stderr, "FFMPEG::init_encoder: failed: %s\n", filename);
		ret = 1;
	}
	if( !ret ) {
		encoding = -1;
		load_options("encode.opts", opts);
	}
	ff_unlock();
	start_muxer();
	return ret;
}

int FFMPEG::open_encoder(const char *path, const char *spec)
{

	Asset *asset = file_base->asset;
	char *filename = asset->path;
	AVDictionary *sopts = 0;
	av_dict_copy(&sopts, opts, 0);
	char option_path[BCTEXTLEN];
	set_option_path(option_path, "%s/%s.opts", path, path);
	read_options(option_path, sopts);
	get_option_path(option_path, path, spec);
	char format_name[BCSTRLEN], codec_name[BCTEXTLEN];
	char bsfilter[BCSTRLEN], bsargs[BCTEXTLEN];
	if( read_options(option_path, format_name, codec_name, bsfilter, bsargs, sopts) ) {
		fprintf(stderr, "FFMPEG::open_encoder: read options failed %s:%s\n",
			option_path, filename);
		return 1;
	}

	int ret = 0;
	ff_lock("FFMPEG::open_encoder");
	FFStream *fst = 0;
	AVStream *st = 0;

	const AVCodecDescriptor *codec_desc = 0;
	AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
	if( !codec ) {
		fprintf(stderr, "FFMPEG::open_encoder: cant find codec %s:%s\n",
			codec_name, filename);
		ret = 1;
	}
	if( !ret ) {
		codec_desc = avcodec_descriptor_get(codec->id);
		if( !codec_desc ) {
			fprintf(stderr, "FFMPEG::open_encoder: unknown codec %s:%s\n",
				codec_name, filename);
			ret = 1;
		}
	}
	if( !ret ) {
		st = avformat_new_stream(fmt_ctx, 0);
		if( !st ) {
			fprintf(stderr, "FFMPEG::open_encoder: cant create stream %s:%s\n",
				codec_name, filename);
			ret = 1;
		}
	} 
	if( !ret ) {
		AVCodecContext *ctx = st->codec;
		switch( codec_desc->type ) {
		case AVMEDIA_TYPE_AUDIO: {
			if( has_audio ) {
				fprintf(stderr, "FFMPEG::open_encoder: duplicate audio %s:%s\n",
					codec_name, filename);
				ret = 1;
				break;
			}
			has_audio = 1;
			int aidx = ffaudio.size();
			int idx = aidx + ffvideo.size();
			FFAudioStream *aud = new FFAudioStream(this, st, idx);
			ffaudio.append(aud);  fst = aud;
			aud->sample_rate = asset->sample_rate;
			ctx->channels = aud->channels = asset->channels;
			for( int ch=0; ch<aud->channels; ++ch )
				astrm_index.append(ffidx(aidx, ch));
			ctx->channel_layout =  av_get_default_channel_layout(ctx->channels);
			ctx->sample_rate = check_sample_rate(codec, asset->sample_rate);
			if( !ctx->sample_rate ) {
				fprintf(stderr, "FFMPEG::open_audio_encode:"
					" check_sample_rate failed %s\n", filename);
				ret = 1;
				break;
			}
			ctx->time_base = st->time_base = (AVRational){1, aud->sample_rate};
			ctx->sample_fmt = codec->sample_fmts[0];
			uint64_t layout = av_get_default_channel_layout(ctx->channels);
			aud->resample_context = swr_alloc_set_opts(NULL,
				layout, ctx->sample_fmt, aud->sample_rate,
				layout, AV_SAMPLE_FMT_FLT, ctx->sample_rate,
				0, NULL);
			swr_init(aud->resample_context);
			aud->writing = -1;
			break; }
		case AVMEDIA_TYPE_VIDEO: {
			if( has_video ) {
				fprintf(stderr, "FFMPEG::open_encoder: duplicate video %s:%s\n",
					codec_name, filename);
				ret = 1;
				break;
			}
			has_video = 1;
			int vidx = ffvideo.size();
			int idx = vidx + ffaudio.size();
			FFVideoStream *vid = new FFVideoStream(this, st, idx);
			vstrm_index.append(ffidx(vidx, 0));
			ffvideo.append(vid);  fst = vid;
			vid->width = asset->width;
			ctx->width = (vid->width+3) & ~3;
			vid->height = asset->height;
			ctx->height = (vid->height+3) & ~3;
			vid->frame_rate = asset->frame_rate;
			ctx->sample_aspect_ratio = to_sample_aspect_ratio(asset->aspect_ratio);
			ctx->pix_fmt = codec->pix_fmts ? codec->pix_fmts[0] : AV_PIX_FMT_YUV420P;
			AVRational frame_rate = check_frame_rate(codec, vid->frame_rate);
			if( !frame_rate.num || !frame_rate.den ) {
				fprintf(stderr, "FFMPEG::open_audio_encode:"
					" check_frame_rate failed %s\n", filename);
				ret = 1;
				break;
			}
			ctx->time_base = (AVRational) { frame_rate.den, frame_rate.num };
			st->time_base = ctx->time_base;
			vid->writing = -1;
			break; }
		default:
			fprintf(stderr, "FFMPEG::open_encoder: not audio/video, %s:%s\n",
				codec_name, filename);
			ret = 1;
		}
	}
	if( !ret ) {
		ret = avcodec_open2(st->codec, codec, &sopts);
		if( ret < 0 ) {
			ff_err(ret,"FFMPEG::open_encoder");
			fprintf(stderr, "FFMPEG::open_encoder: open failed %s:%s\n",
				codec_name, filename);
			ret = 1;
		}
		else
			ret = 0;
	}
	if( !ret ) {
		if( fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER )
			st->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		if( fst && bsfilter[0] )
			fst->add_bsfilter(bsfilter, !bsargs[0] ? 0 : bsargs);
	}

	ff_unlock();
	av_dict_free(&sopts);
	return ret;
}

int FFMPEG::close_encoder()
{
	stop_muxer();
	if( encoding > 0 ) {
		av_write_trailer(fmt_ctx);
		if( !(fmt_ctx->flags & AVFMT_NOFILE) )
			avio_closep(&fmt_ctx->pb);
	}
	encoding = 0;
	return 0;
}

int FFMPEG::decode_activate()
{
	if( decoding < 0 ) {
		decoding = 0;
		int npgrms = fmt_ctx->nb_programs;
		for( int i=0; i<npgrms; ++i ) {
			AVProgram *pgrm = fmt_ctx->programs[i];
			// first start time video stream
			int64_t vstart_time = -1;
			for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j ) {
				int st_idx = pgrm->stream_index[j];
				AVStream *st = fmt_ctx->streams[st_idx];
				AVCodecContext *avctx = st->codec;
				if( avctx->codec_type == AVMEDIA_TYPE_VIDEO ) {
					if( st->start_time == AV_NOPTS_VALUE ) continue;
					vstart_time = st->start_time;
					break;
				}
			}
			// max start time audio stream
			int64_t astart_time = -1;
			for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j ) {
				int st_idx = pgrm->stream_index[j];
				AVStream *st = fmt_ctx->streams[st_idx];
				AVCodecContext *avctx = st->codec;
				if( avctx->codec_type == AVMEDIA_TYPE_VIDEO ) {
					if( st->start_time == AV_NOPTS_VALUE ) continue;
					if( astart_time > st->start_time ) continue;
					astart_time = st->start_time;
				}
			}
			if( astart_time < 0 || vstart_time < 0 ) continue;
			// match program streams to max start_time
			int64_t nudge = vstart_time > astart_time ? vstart_time : astart_time;
			for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j ) {
				int st_idx = pgrm->stream_index[j];
				AVStream *st = fmt_ctx->streams[st_idx];
				AVCodecContext *avctx = st->codec;
				if( avctx->codec_type == AVMEDIA_TYPE_AUDIO ) {
					for( int k=0; k<ffaudio.size(); ++k ) {
						if( ffaudio[k]->idx == st_idx )
							ffaudio[k]->nudge = nudge;
					}
				}
				else if( avctx->codec_type == AVMEDIA_TYPE_VIDEO ) {
					for( int k=0; k<ffvideo.size(); ++k ) {
						if( ffvideo[k]->idx == st_idx )
							ffvideo[k]->nudge = nudge;
					}
				}
			}
		}
		int64_t vstart_time = 0, astart_time = 0;
		int nstreams = fmt_ctx->nb_streams;
		for( int i=0; i<nstreams; ++i ) {
			AVStream *st = fmt_ctx->streams[i];
			AVCodecContext *avctx = st->codec;
			switch( avctx->codec_type ) {
			case AVMEDIA_TYPE_VIDEO:
				if( st->start_time == AV_NOPTS_VALUE ) continue;
				if( vstart_time >= st->start_time ) continue;
				vstart_time = st->start_time;
				break;
			case AVMEDIA_TYPE_AUDIO:
				if( st->start_time == AV_NOPTS_VALUE ) continue;
				if( astart_time >= st->start_time ) continue;
				astart_time = st->start_time;
			default: break;
			}
		}
		int64_t nudge = vstart_time > astart_time ? vstart_time : astart_time;
		for( int k=0; k<ffvideo.size(); ++k ) {
			if( ffvideo[k]->nudge != AV_NOPTS_VALUE ) continue;
			ffvideo[k]->nudge = nudge;
		}
		for( int k=0; k<ffaudio.size(); ++k ) {
			if( ffaudio[k]->nudge != AV_NOPTS_VALUE ) continue;
			ffaudio[k]->nudge = nudge;
		}
		decoding = 1;
	}
	return decoding;
}

int FFMPEG::encode_activate()
{
	if( encoding < 0 ) {
		encoding = 0;
		if( !(fmt_ctx->flags & AVFMT_NOFILE) &&
		    avio_open(&fmt_ctx->pb, fmt_ctx->filename, AVIO_FLAG_WRITE) < 0 ) {
			fprintf(stderr, "FFMPEG::encode_activate: err opening : %s\n",
				fmt_ctx->filename);
			return 1;
		}

		AVDictionary *fopts = 0;
		char option_path[BCTEXTLEN];
		set_option_path(option_path, "format/%s", file_format);
		read_options(option_path, fopts);
		int ret = avformat_write_header(fmt_ctx, &fopts);
		av_dict_free(&fopts);
		if( ret < 0 ) {
			fprintf(stderr, "FFMPEG::encode_activate: write header failed %s\n",
				fmt_ctx->filename);
			return 1;
		}
		encoding = 1;
	}
	return encoding;
}

int FFMPEG::audio_seek(int stream, int64_t pos)
{
	int aidx = astrm_index[stream].st_idx;
	FFAudioStream *aud = ffaudio[aidx];
	aud->audio_seek(pos);
	aud->seek_pos = aud->curr_pos = pos;
	return 0;
}

int FFMPEG::video_seek(int stream, int64_t pos)
{
	int vidx = vstrm_index[stream].st_idx;
	FFVideoStream *vid = ffvideo[vidx];
	vid->video_seek(pos);
	vid->seek_pos = vid->curr_pos = pos;
	return 0;
}


int FFMPEG::decode(int chn, int64_t pos, double *samples, int len)
{
	if( !has_audio || chn >= astrm_index.size() ) return -1;
	int aidx = astrm_index[chn].st_idx;
	FFAudioStream *aud = ffaudio[aidx];
	if( aud->load(pos, len) < len ) return -1;
	int ch = astrm_index[chn].st_ch;
	return aud->history.read(samples,len,ch);
}

int FFMPEG::decode(int layer, int64_t pos, VFrame *vframe)
{
	if( !has_video || layer >= vstrm_index.size() ) return -1;
	int vidx = vstrm_index[layer].st_idx;
	FFVideoStream *vid = ffvideo[vidx];
	return vid->load(vframe, pos);
}

int FFMPEG::encode(int stream, double **samples, int len)
{
	FFAudioStream *aud = ffaudio[stream];
	return aud->encode(samples, len);
}


int FFMPEG::encode(int stream, VFrame *frame)
{
	FFVideoStream *vid = ffvideo[stream];
	return vid->encode(frame);
}

void FFMPEG::start_muxer()
{
	if( !running() ) {
		done = 0;
		start();
	}
}

void FFMPEG::stop_muxer()
{
	if( running() ) {
		done = 1;
		mux_lock->unlock();
		join();
	}
}

void FFMPEG::flow_off()
{
	if( !flow ) return;
	flow_lock->lock("FFMPEG::flow_off");
	flow = 0;
}

void FFMPEG::flow_on()
{
	if( flow ) return;
	flow = 1;
	flow_lock->unlock();
}

void FFMPEG::flow_ctl()
{
	while( !flow ) {
		flow_lock->lock("FFMPEG::flow_ctl");
		flow_lock->unlock();
	}
}

int FFMPEG::mux_audio(FFrame *frm)
{
	FFPacket pkt;
	AVStream *st = frm->fst->st;
	AVCodecContext *ctx = st->codec;
	AVFrame *frame = *frm;
	AVRational tick_rate = {1, ctx->sample_rate};
	frame->pts = av_rescale_q(frm->position, tick_rate, ctx->time_base);
	int got_packet = 0;
	int ret = avcodec_encode_audio2(ctx, pkt, frame, &got_packet);
	if( ret >= 0 && got_packet ) {
		frm->fst->bs_filter(pkt);
		av_packet_rescale_ts(pkt, ctx->time_base, st->time_base);
		pkt->stream_index = st->index;
		ret = av_interleaved_write_frame(fmt_ctx, pkt);
	}
	if( ret < 0 )
		ff_err(ret, "FFMPEG::mux_audio");
	return ret >= 0 ? 0 : 1;
}

int FFMPEG::mux_video(FFrame *frm)
{
	FFPacket pkt;
	AVStream *st = frm->fst->st;
	AVFrame *frame = *frm;
	frame->pts = frm->position;
	int ret = 1, got_packet = 0;
	if( fmt_ctx->oformat->flags & AVFMT_RAWPICTURE ) {
		/* a hack to avoid data copy with some raw video muxers */
		pkt->flags |= AV_PKT_FLAG_KEY;
		pkt->stream_index  = st->index;
		AVPicture *picture = (AVPicture *)frame;
		pkt->data = (uint8_t *)picture;
		pkt->size = sizeof(AVPicture);
		pkt->pts = pkt->dts = frame->pts;
		got_packet = 1;
	}
	else
		ret = avcodec_encode_video2(st->codec, pkt, frame, &got_packet);
	if( ret >= 0 && got_packet ) {
		frm->fst->bs_filter(pkt);
		av_packet_rescale_ts(pkt, st->codec->time_base, st->time_base);
		pkt->stream_index = st->index;
		ret = av_interleaved_write_frame(fmt_ctx, pkt);
	}
	if( ret < 0 )
		ff_err(ret, "FFMPEG::mux_video");
	return ret >= 0 ? 0 : 1;
}

void FFMPEG::mux()
{
	for(;;) {
		double atm = -1, vtm = -1;
		FFrame *afrm = 0, *vfrm = 0;
		int demand = 0;
		for( int i=0; i<ffaudio.size(); ++i ) {  // earliest audio
			FFStream *fst = ffaudio[i];
			if( fst->frm_count < 3 ) { demand = 1; flow_on(); }
			FFrame *frm = fst->frms.first;
			if( !frm ) { if( !done ) return; continue; }
			double tm = to_secs(frm->position, fst->st->codec->time_base);
			if( atm < 0 || tm < atm ) { atm = tm;  afrm = frm; }
		}
		for( int i=0; i<ffvideo.size(); ++i ) {  // earliest video
			FFStream *fst = ffvideo[i];
			if( fst->frm_count < 2 ) { demand = 1; flow_on(); }
			FFrame *frm = fst->frms.first;
			if( !frm ) { if( !done ) return; continue; }
			double tm = to_secs(frm->position, fst->st->codec->time_base);
			if( vtm < 0 || tm < vtm ) { vtm = tm;  vfrm = frm; }
		}
		if( !demand ) flow_off();
		if( !afrm && !vfrm ) break;
		int v = !afrm ? -1 : !vfrm ? 1 : av_compare_ts(
			vfrm->position, vfrm->fst->st->codec->time_base,
			afrm->position, afrm->fst->st->codec->time_base);
		FFrame *frm = v <= 0 ? vfrm : afrm;
		if( frm == afrm ) mux_audio(frm);
		if( frm == vfrm ) mux_video(frm);
		frm->dequeue();
		delete frm;
	}
}

void FFMPEG::run()
{
	while( !done ) {
		mux_lock->lock("FFMPEG::run");
		if( !done ) mux();
	}
	mux();
}


int FFMPEG::ff_total_audio_channels()
{
	return astrm_index.size();
}

int FFMPEG::ff_total_astreams()
{
	return ffaudio.size();
}

int FFMPEG::ff_audio_channels(int stream)
{
	return ffaudio[stream]->channels;
}

int FFMPEG::ff_sample_rate(int stream)
{
	return ffaudio[stream]->sample_rate;
}

const char* FFMPEG::ff_audio_format(int stream)
{
	AVStream *st = ffaudio[stream]->st;
	AVCodecID id = st->codec->codec_id;
	const AVCodecDescriptor *desc = avcodec_descriptor_get(id);
	return desc ? desc->name : "Unknown";
}

int FFMPEG::ff_audio_pid(int stream)
{
	return ffaudio[stream]->st->id;
}

int64_t FFMPEG::ff_audio_samples(int stream)
{
	return ffaudio[stream]->length;
}

// find audio astream/channels with this program,
//   or all program audio channels (astream=-1)
int FFMPEG::ff_audio_for_video(int vstream, int astream, int64_t &channel_mask)
{
	channel_mask = 0;
	int pidx = -1;
	int vidx = ffvideo[vstream]->idx;
	// find first program with this video stream
	for( int i=0; pidx<0 && i<(int)fmt_ctx->nb_programs; ++i ) {
		AVProgram *pgrm = fmt_ctx->programs[i];
		for( int j=0;  pidx<0 && j<(int)pgrm->nb_stream_indexes; ++j ) {
			int st_idx = pgrm->stream_index[j];
			AVStream *st = fmt_ctx->streams[st_idx];
			if( st->codec->codec_type != AVMEDIA_TYPE_VIDEO ) continue;
			if( st_idx == vidx ) pidx = i;
		}
	}
	if( pidx < 0 ) return -1;
	int ret = -1;
	int64_t channels = 0;
	AVProgram *pgrm = fmt_ctx->programs[pidx];
	for( int j=0; j<(int)pgrm->nb_stream_indexes; ++j ) {
		int aidx = pgrm->stream_index[j];
		AVStream *st = fmt_ctx->streams[aidx];
		if( st->codec->codec_type != AVMEDIA_TYPE_AUDIO ) continue;
		if( astream > 0 ) { --astream;  continue; }
		int astrm = -1;
		for( int i=0; astrm<0 && i<ffaudio.size(); ++i )
			if( ffaudio[i]->idx == aidx ) astrm = i;
		if( astrm >= 0 ) {
			if( ret < 0 ) ret = astrm;
			int64_t mask = (1 << ffaudio[astrm]->channels) - 1;
			channels |= mask << ffaudio[astrm]->channel0;
		}
		if( !astream ) break;
	}
	channel_mask = channels;
	return ret;
}


int FFMPEG::ff_total_video_layers()
{
	return vstrm_index.size();
}

int FFMPEG::ff_total_vstreams()
{
	return ffvideo.size();
}

int FFMPEG::ff_video_width(int stream)
{
	return ffvideo[stream]->width;
}

int FFMPEG::ff_video_height(int stream)
{
	return ffvideo[stream]->height;
}

int FFMPEG::ff_set_video_width(int stream, int width)
{
	int w = ffvideo[stream]->width;
	ffvideo[stream]->width = width;
	return w;
}

int FFMPEG::ff_set_video_height(int stream, int height)
{
	int h = ffvideo[stream]->height;
	ffvideo[stream]->height = height;
	return h;
}

int FFMPEG::ff_coded_width(int stream)
{
	AVStream *st = ffvideo[stream]->st;
	return st->codec->coded_width;
}

int FFMPEG::ff_coded_height(int stream)
{
	AVStream *st = ffvideo[stream]->st;
	return st->codec->coded_height;
}

float FFMPEG::ff_aspect_ratio(int stream)
{
	return ffvideo[stream]->aspect_ratio;
}

const char* FFMPEG::ff_video_format(int stream)
{
	AVStream *st = ffvideo[stream]->st;
	AVCodecID id = st->codec->codec_id;
	const AVCodecDescriptor *desc = avcodec_descriptor_get(id);
	return desc ? desc->name : "Unknown";
}

double FFMPEG::ff_frame_rate(int stream)
{
	return ffvideo[stream]->frame_rate;
}

int64_t FFMPEG::ff_video_frames(int stream)
{
	return ffvideo[stream]->length;
}

int FFMPEG::ff_video_pid(int stream)
{
	return ffvideo[stream]->st->id;
}


int FFMPEG::ff_cpus()
{
	return file_base->file->cpus;
}

int FFVideoStream::create_filter(const char *filter_spec,
		AVCodecContext *src_ctx, AVCodecContext *sink_ctx)
{
	avfilter_register_all();
	filter_graph = avfilter_graph_alloc();
	AVFilter *buffersrc = avfilter_get_by_name("buffer");
	AVFilter *buffersink = avfilter_get_by_name("buffersink");

	int ret = 0;  char args[BCTEXTLEN];
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		src_ctx->width, src_ctx->height, src_ctx->pix_fmt,
		st->time_base.num, st->time_base.den,
		src_ctx->sample_aspect_ratio.num, src_ctx->sample_aspect_ratio.den);
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
			args, NULL, filter_graph);
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
			NULL, NULL, filter_graph);
	if( ret >= 0 )
		ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
			(uint8_t*)&sink_ctx->pix_fmt, sizeof(sink_ctx->pix_fmt),
			AV_OPT_SEARCH_CHILDREN);
	if( ret < 0 )
		ff_err(ret, "FFVideoStream::create_filter");
	else
		ret = FFStream::create_filter(filter_spec);
	return ret >= 0 ? 0 : 1;
}

int FFAudioStream::create_filter(const char *filter_spec,
		AVCodecContext *src_ctx, AVCodecContext *sink_ctx)
{
	avfilter_register_all();
	filter_graph = avfilter_graph_alloc();
	AVFilter *buffersrc = avfilter_get_by_name("abuffer");
	AVFilter *buffersink = avfilter_get_by_name("abuffersink");
	int ret = 0;  char args[BCTEXTLEN];
	snprintf(args, sizeof(args),
		"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%jx",
		st->time_base.num, st->time_base.den, src_ctx->sample_rate,
		av_get_sample_fmt_name(src_ctx->sample_fmt), src_ctx->channel_layout);
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
			args, NULL, filter_graph);
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
			NULL, NULL, filter_graph);
	if( ret >= 0 )
		ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
			(uint8_t*)&sink_ctx->sample_fmt, sizeof(sink_ctx->sample_fmt),
			AV_OPT_SEARCH_CHILDREN);
	if( ret >= 0 )
		ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
			(uint8_t*)&sink_ctx->channel_layout,
			sizeof(sink_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
	if( ret >= 0 )
		ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
			(uint8_t*)&sink_ctx->sample_rate, sizeof(sink_ctx->sample_rate),
			AV_OPT_SEARCH_CHILDREN);
	if( ret < 0 )
		ff_err(ret, "FFAudioStream::create_filter");
	else
		ret = FFStream::create_filter(filter_spec);
	return ret >= 0 ? 0 : 1;
}

int FFStream::create_filter(const char *filter_spec)
{
	/* Endpoints for the filter graph. */
	AVFilterInOut *outputs = avfilter_inout_alloc();
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = 0;

	AVFilterInOut *inputs  = avfilter_inout_alloc();
	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx	= 0;
	inputs->next = 0;

	int ret = !outputs->name || !inputs->name ? -1 : 0;
	if( ret >= 0 )
		ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
			&inputs, &outputs, NULL);
	if( ret >= 0 )
		ret = avfilter_graph_config(filter_graph, NULL);

	if( ret < 0 )
		ff_err(ret, "FFStream::create_filter");
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	return ret;
}

void FFStream::add_bsfilter(const char *bsf, const char *ap)
{
	bsfilter.append(new BSFilter(bsf,ap));
}

int FFStream::bs_filter(AVPacket *pkt)
{
	if( !bsfilter.size() ) return 0;
	av_packet_split_side_data(pkt);

	int ret = 0;
	for( int i=0; i<bsfilter.size(); ++i ) {
		AVPacket bspkt = *pkt;
		ret = av_bitstream_filter_filter(bsfilter[i]->bsfc,
			 st->codec, bsfilter[i]->args, &bspkt.data, &bspkt.size,
			 pkt->data, pkt->size, pkt->flags & AV_PKT_FLAG_KEY);
		if( ret < 0 ) break;
		int size = bspkt.size;
		uint8_t *data = bspkt.data;
		if( !ret && bspkt.data != pkt->data ) {
			size = bspkt.size;
			data = (uint8_t *)av_malloc(size + FF_INPUT_BUFFER_PADDING_SIZE);
			if( !data ) { ret = AVERROR(ENOMEM);  break; }
			memcpy(data, bspkt.data, size);
			memset(data+size, 0, FF_INPUT_BUFFER_PADDING_SIZE);
			ret = 1;
		}
		if( ret > 0 ) {
			pkt->side_data = 0;  pkt->side_data_elems = 0;
			av_free_packet(pkt);
			ret = av_packet_from_data(&bspkt, data, size);
			if( ret < 0 ) break;
		}
		*pkt = bspkt;
	}
	if( ret < 0 )
		ff_err(ret,"FFStream::bs_filter");
	return ret;
}

