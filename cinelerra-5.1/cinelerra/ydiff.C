#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/time.h>

extern "C" {
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#include <gtk/gtk.h>
#include <gdk/gdk.h>

int done = 0;

void sigint(int n)
{
  done = 1;
}


void dst_exit(GtkWidget *widget, gpointer data)
{
   exit(0);
}

class gtk_window {
public:
  gtk_window(int width, int height);
  ~gtk_window();

  GdkVisual *visual;
  GtkWidget *window;
  GtkWidget *image;
  GtkWidget *panel_hbox;
  GdkImage  *img0, *img1;
  GdkPixbuf *pbuf0, *pbuf1;
  unsigned char *bfr, *bfrs, *bfr0, *bfr1;
  unsigned char **rows, **row0, **row1;
  uint64_t flip_bfrs, flip_rows;
  int width, height, linesize;
  int done;

  pthread_t tid;
  static void *entry(void *t);
  void start();
  void *run();
  uint8_t *next_bfr();
  void post();

  pthread_mutex_t draw;
  void draw_lock() { pthread_mutex_lock(&draw); }
  void draw_unlock() { pthread_mutex_unlock(&draw); }
};

gtk_window::gtk_window(int width, int height)
{
  this->width = width;
  this->height = height;
  this->linesize = width*3;
  visual = gdk_visual_get_system();
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(window),"destroy",
     GTK_SIGNAL_FUNC(dst_exit),NULL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
  /* try for shared image bfr, only seems to work with gtk_rgb */
  img0 = gdk_image_new(GDK_IMAGE_SHARED, visual, width, height);
  pbuf0 = gdk_pixbuf_new_from_data((const guchar *)img0->mem,
     GDK_COLORSPACE_RGB,FALSE,8,width,height,linesize,NULL,NULL);
  bfr0 = gdk_pixbuf_get_pixels(pbuf0);
  memset(bfr0,0,height*linesize);
  image = gtk_image_new_from_pixbuf(pbuf0);
  /* double buffered */
  img1 = gdk_image_new(GDK_IMAGE_SHARED, visual, width, height);
  pbuf1 = gdk_pixbuf_new_from_data((const guchar *)img1->mem,
     GDK_COLORSPACE_RGB,FALSE,8,width,height,linesize,NULL,NULL);
  bfr1 = gdk_pixbuf_get_pixels(pbuf1);
  memset(bfr1,0,height*linesize);

  row0 = new unsigned char *[height];
  row1 = new unsigned char *[height];

  for( int i=0; i<height; ++i ) {
    row0[i] = bfr0 + i*linesize;
    row1[i] = bfr1 + i*linesize;
  }
  bfrs = bfr0;
  rows = row0;

  flip_bfrs = ((uint64_t)bfr0 ^ (uint64_t)bfr1);
  flip_rows = ((uint64_t)row0 ^ (uint64_t)row1);
  pthread_mutex_init(&draw, 0);
  post();
  
  panel_hbox = gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(window), panel_hbox);
  /* pack image into panel */
  gtk_box_pack_start(GTK_BOX(panel_hbox), image, TRUE, TRUE, 0);
  gtk_widget_show_all(window);
}

gtk_window::~gtk_window()
{
  gtk_widget_destroy(window);
  g_object_unref(pbuf0);
  g_object_unref(img0);
  g_object_unref(pbuf1);
  g_object_unref(img1);
  delete [] row0;
  delete [] row1;
  pthread_mutex_destroy(&draw);
}

void *gtk_window::entry(void *t)
{
  return ((gtk_window*)t)->run();
}

void gtk_window::start()
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
  done = 0;
  pthread_create(&tid, &attr, &entry, this);
  pthread_attr_destroy(&attr);
}

void *gtk_window::run()
{
  while( !done ) {
    if( !gtk_events_pending() ) {
      if( !bfr ) { usleep(10000);  continue; }
      GdkGC *blk = image->style->black_gc;
      gdk_draw_rgb_image(image->window,blk, 0,0,width,height,
         GDK_RGB_DITHER_NONE,bfr,linesize);
      gdk_flush();
      unsigned long *fbfrs = (unsigned long *)&bfrs;  *fbfrs ^= flip_bfrs;
      unsigned long *frows = (unsigned long *)&rows;  *frows ^= flip_rows;
      bfr = 0;
      draw_unlock();
    }
    else
      gtk_main_iteration();
  }

  return (void*)0;
}

uint8_t *gtk_window::next_bfr()
{
  return bfrs;
}

void gtk_window::post()
{
  draw_lock();
  bfr = bfrs;
}


class ffcmpr {
public:
  ffcmpr();
  ~ffcmpr();
  AVPacket ipkt;
  AVFormatContext *fmt_ctx;
  AVFrame *ipic;
  AVStream *st;
  AVCodecContext *ctx;
  AVPixelFormat pix_fmt;
  double frame_rate;
  int width, height;
  int need_packet, eof;
  int open_decoder(const char *filename, int vid_no);
  void close_decoder();
  AVFrame *read_frame();
};

ffcmpr::ffcmpr()
{
  av_init_packet(&this->ipkt);
  this->fmt_ctx = 0;
  this->ipic = 0;
  this->st = 0;
  this->ctx = 0 ;
  this->frame_rate = 0;
  this->need_packet = 0;
  this->eof = 0;
  this->pix_fmt = AV_PIX_FMT_NONE;
  width = height = 0;
}

void ffcmpr::close_decoder()
{
  av_packet_unref(&ipkt);
  if( !fmt_ctx ) return;
  if( ctx ) avcodec_free_context(&ctx);
  avformat_close_input(&fmt_ctx);
  av_frame_free(&ipic);
}

ffcmpr::~ffcmpr()
{
  close_decoder();
}

int ffcmpr::open_decoder(const char *filename, int vid_no)
{
  struct stat fst;
  if( stat(filename, &fst) ) return 1;

  av_log_set_level(AV_LOG_VERBOSE);
  fmt_ctx = 0;
  AVDictionary *fopts = 0;
  av_register_all();
  av_dict_set(&fopts, "formatprobesize", "5000000", 0);
  av_dict_set(&fopts, "scan_all_pmts", "1", 0);
  av_dict_set(&fopts, "threads", "auto", 0);
  int ret = avformat_open_input(&fmt_ctx, filename, NULL, &fopts);
  av_dict_free(&fopts);
  if( ret < 0 ) {
    fprintf(stderr,"file open failed: %s\n", filename);
    return ret;
  }
  ret = avformat_find_stream_info(fmt_ctx, NULL);
  if( ret < 0 ) {
    fprintf(stderr,"file probe failed: %s\n", filename);
    return ret;
  }

  this->st = 0;
  for( int i=0; !this->st && ret>=0 && i<(int)fmt_ctx->nb_streams; ++i ) {
    AVStream *fst = fmt_ctx->streams[i];
    AVMediaType type = fst->codecpar->codec_type;
    if( type != AVMEDIA_TYPE_VIDEO ) continue;
    if( --vid_no < 0 ) this->st = fst;
  }

  AVCodecID codec_id = st->codecpar->codec_id;
  AVDictionary *copts = 0;
  //av_dict_copy(&copts, opts, 0);
  AVCodec *decoder = avcodec_find_decoder(codec_id);
  ctx = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(ctx, st->codecpar);
  if( avcodec_open2(ctx, decoder, &copts) < 0 ) {
    fprintf(stderr,"codec open failed: %s\n", filename);
    return -1;
  }
  av_dict_free(&copts);
  ipic = av_frame_alloc();
  eof = 0;
  need_packet = 1;

  AVRational framerate = av_guess_frame_rate(fmt_ctx, st, 0);
  this->frame_rate = !framerate.den ? 0 : (double)framerate.num / framerate.den;
  this->pix_fmt = (AVPixelFormat)st->codecpar->format;
  this->width  = st->codecpar->width;
  this->height = st->codecpar->height;
  return 0;
}

AVFrame *ffcmpr::read_frame()
{
  av_frame_unref(ipic);

  for( int retrys=1000; --retrys>=0; ) {
    if( need_packet ) {
      if( eof ) return 0;
      AVPacket *pkt = &ipkt;
      av_packet_unref(pkt);
      int ret = av_read_frame(fmt_ctx, pkt);
      if( ret < 0 ) {
        if( ret != AVERROR_EOF ) return 0;
        ret = 0;  eof = 1;  pkt = 0;
      }
      if( pkt ) {
        if( pkt->stream_index != st->index ) continue;
        if( !pkt->data || !pkt->size ) continue;
      }
      avcodec_send_packet(ctx, pkt);
      need_packet = 0;
    }
    int ret = avcodec_receive_frame(ctx, ipic);
    if( ret >= 0 ) return ipic;
    if( ret != AVERROR(EAGAIN) ) {
      eof = 1; need_packet = 0;
      break;
    }
    need_packet = 1;
  }
  return 0;
}

static int diff_frame(AVFrame *afrm, AVFrame *bfrm, uint8_t *fp, int w, int h)
{
  int n = 0, m = 0;
  uint8_t *arow = afrm->data[0];
  uint8_t *brow = bfrm->data[0];
  int asz = afrm->linesize[0], bsz = afrm->linesize[0];
  int rsz = w * 3;
  for( int y=h; --y>=0; arow+=asz, brow+=bsz ) {
    uint8_t *ap = arow, *bp = brow;
    for( int x=rsz; --x>=0; ++ap,++bp ) {
      int d = *ap - *bp, v = d + 128;
      if( v > 255 ) v = 255;
      else if( v < 0 ) v = 0;
      *fp++ = v;
      m += d;
      if( d < 0 ) d = -d;
      n += d;
    }
  }
  int sz = h*rsz;
  printf("%d %d %d %f", sz, m, n, (double)n/sz);
  return n;
}

int main(int ac, char **av)
{
  int ret;
  setbuf(stdout,NULL);

  gtk_set_locale();
  gtk_init(&ac, &av);

  ffcmpr a, b;
  if( a.open_decoder(av[1],0) ) return 1;
  if( b.open_decoder(av[2],0) ) return 1;

  printf("file a:%s\n", av[1]);
  printf("  id 0x%06x:", a.ctx->codec_id);
  const AVCodecDescriptor *adesc = avcodec_descriptor_get(a.ctx->codec_id);
  printf("  video %s\n", adesc ? adesc->name : " (unkn)");
  printf(" %dx%d %5.2f", a.width, a.height, a.frame_rate);
  const char *apix = av_get_pix_fmt_name(a.pix_fmt);
  printf(" pix %s\n", apix ? apix : "(unkn)");

  printf("file b:%s\n", av[2]);
  printf("  id 0x%06x:", b.ctx->codec_id);
  const AVCodecDescriptor *bdesc = avcodec_descriptor_get(b.ctx->codec_id);
  printf("  video %s\n", bdesc ? bdesc->name : " (unkn)");
  printf(" %dx%d %5.2f", b.width, b.height, b.frame_rate);
  const char *bpix = av_get_pix_fmt_name(b.pix_fmt);
  printf(" pix %s\n", bpix ? bpix : "(unkn)");

//  if( a.ctx->codec_id != b.ctx->codec_id ) { printf("codec mismatch\n"); return 1;}
  if( a.width != b.width ) { printf("width mismatch\n"); return 1;}
  if( a.height != b.height ) { printf("height mismatch\n"); return 1;}
  if( a.frame_rate != b.frame_rate ) { printf("framerate mismatch\n"); return 1;}
//  if( a.pix_fmt != b.pix_fmt ) { printf("format mismatch\n"); return 1;}

  signal(SIGINT,sigint);

  struct SwsContext *a_cvt = sws_getCachedContext(0, a.width, a.height, a.pix_fmt,
                a.width, a.height, AV_PIX_FMT_RGB24, SWS_POINT, 0, 0, 0);
  struct SwsContext *b_cvt = sws_getCachedContext(0, b.width, b.height, b.pix_fmt,
                b.width, b.height, AV_PIX_FMT_RGB24, SWS_POINT, 0, 0, 0);
  if( !a_cvt || !b_cvt ) {
    printf("sws_getCachedContext() failed\n");
    return 1;
  }

  AVFrame *afrm = av_frame_alloc();
  av_image_alloc(afrm->data, afrm->linesize,
     a.width, a.height, AV_PIX_FMT_RGB24, 1);

  AVFrame *bfrm = av_frame_alloc();
  av_image_alloc(bfrm->data, bfrm->linesize,
     b.width, b.height, AV_PIX_FMT_RGB24, 1);

  gtk_window gw(a.width, a.height);
  gw.start();

  int64_t err = 0;
  int frm_no = 0;

  if( ac>3 && (ret=atoi(av[3])) ) {
    while( ret > 0 ) { a.read_frame(); --ret; }
    while( ret < 0 ) { b.read_frame(); ++ret; }
  }

  while( !done ) {
    AVFrame *ap = a.read_frame();
    if( !ap ) break;
    AVFrame *bp = b.read_frame();
    if( !bp ) break;
    sws_scale(a_cvt, ap->data, ap->linesize, 0, ap->height,
       afrm->data, afrm->linesize);
    sws_scale(b_cvt, bp->data, bp->linesize, 0, bp->height,
       bfrm->data, bfrm->linesize);
    uint8_t *fbfr = gw.next_bfr();
    ret = diff_frame(afrm, bfrm, fbfr, ap->width, ap->height);
    err += ret;  ++frm_no;
    printf("  %d\n",frm_no);
    gw.post();
  }

  av_freep(&afrm->data);
  av_frame_free(&afrm);
  av_freep(&bfrm->data);
  av_frame_free(&bfrm);
  
  b.close_decoder();
  a.close_decoder();
  return 0;
}

