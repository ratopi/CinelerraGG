#include "bccmodels.h"
#include "clip.h"

static inline float clp(const int n, float v) {
 v *= ((float)(n*(1-1./0x1000000)));
 return v < 0 ? 0 : v >= n ? n-1 : v;
}

static inline float fclp(float v, const int n) {
 v /= ((float)(n*(1-1./0x1000000)));
 return v < 0 ? 0 : v > 1 ? 1 : v;
}

#include <stdint.h>

#define ZTYP(ty) typedef ty z_##ty __attribute__ ((__unused__))
ZTYP(int);
ZTYP(float);

// All variables are unsigned
// y -> 24 bits u, v, -> 8 bits r, g, b -> 8 bits
#define YUV_TO_RGB(y, u, v, r, g, b) \
{ \
	(r) = ((y + vtor_tab[v]) >> 16); \
	(g) = ((y + utog_tab[u] + vtog_tab[v]) >> 16); \
	(b) = ((y + utob_tab[u]) >> 16); \
	CLAMP(r, 0, 0xff); CLAMP(g, 0, 0xff); CLAMP(b, 0, 0xff); \
}

// y -> 0 - 1 float
// u, v, -> 8 bits
// r, g, b -> float
#define YUV_TO_FLOAT(y, u, v, r, g, b) \
{ \
	(r) = y + vtor_float_tab[v]; \
	(g) = y + utog_float_tab[u] + vtog_float_tab[v]; \
	(b) = y + utob_float_tab[u]; \
}

// y -> 0 - 1 float
// u, v, -> 16 bits
// r, g, b -> float
#define YUV16_TO_RGB_FLOAT(y, u, v, r, g, b) \
{ \
	(r) = y + v16tor_float_tab[v]; \
	(g) = y + u16tog_float_tab[u] + v16tog_float_tab[v]; \
	(b) = y + u16tob_float_tab[u]; \
}

// y -> 24 bits   u, v-> 16 bits
#define YUV_TO_RGB16(y, u, v, r, g, b) \
{ \
	(r) = ((y + vtor_tab16[v]) >> 8); \
	(g) = ((y + utog_tab16[u] + vtog_tab16[v]) >> 8); \
	(b) = ((y + utob_tab16[u]) >> 8); \
	CLAMP(r, 0, 0xffff); CLAMP(g, 0, 0xffff); CLAMP(b, 0, 0xffff); \
}




#define RGB_TO_YUV(y, u, v, r, g, b) \
{ \
	y = ((rtoy_tab[r] + gtoy_tab[g] + btoy_tab[b]) >> 16); \
	u = ((rtou_tab[r] + gtou_tab[g] + btou_tab[b]) >> 16); \
	v = ((rtov_tab[r] + gtov_tab[g] + btov_tab[b]) >> 16); \
	CLAMP(y, 0, 0xff); CLAMP(u, 0, 0xff); CLAMP(v, 0, 0xff); \
}

// r, g, b -> 16 bits
#define RGB_TO_YUV16(y, u, v, r, g, b) \
{ \
	y = ((rtoy_tab16[r] + gtoy_tab16[g] + btoy_tab16[b]) >> 8); \
	u = ((rtou_tab16[r] + gtou_tab16[g] + btou_tab16[b]) >> 8); \
	v = ((rtov_tab16[r] + gtov_tab16[g] + btov_tab16[b]) >> 8); \
	CLAMP(y, 0, 0xffff); CLAMP(u, 0, 0xffff); CLAMP(v, 0, 0xffff); \
}


#define xfer_flat_row_out(oty_t) \
  for( unsigned i=0; i<out_h; ++i ) { \
    oty_t *out = (oty_t *)(output_rows[i + out_y] + out_x * out_pixelsize); \

#define xfer_flat_row_in(ity_t) \
    uint8_t *inp_row = input_rows[row_table[i]]; \
    for( unsigned j=0; j<out_w; ++j ) { \
      ity_t *inp = (ity_t *)(inp_row + column_table[j]); \

#define xfer_end } }

// yuv420p  2x2
#define xfer_yuv420p_row_out(oty_t) \
  for( unsigned i=0; i<out_h; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    out_rofs = i / 2 * total_out_w / 2 + out_x / 2; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv420p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    in_rofs = row_table[i] / 2 * total_in_w / 2; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 2; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv422p  2x1
#define xfer_yuv422p_row_out(oty_t) \
  for( unsigned i=0; i<out_h; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    out_rofs = i * total_out_w / 2 + out_x / 2; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv422p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    in_rofs /= 2; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 2; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv444p  1x1
#define xfer_yuv444p_row_out(oty_t) \
  for( unsigned i=0; i<out_h; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)((oty_t *)(out_yp + out_rofs)); \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv444p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv411p  4x1
#define xfer_yuv411p_row_out(oty_t) \
  for( unsigned i=0; i<out_h; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    out_rofs = i * total_out_w / 4 + out_x / 4; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv411p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    in_rofs /= 4; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 4; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv410p  4x4
#define xfer_yuv410p_row_out(oty_t) \
  for( unsigned i=0; i<out_h; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    out_rofs = i / 4 * total_out_w / 4 + out_x / 4; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv410p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    in_rofs = row_table[i] / 4 * total_in_w / 4; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 4; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// rgb_floatp
#define xfer_rgb_fltp_row_out(oty_t) \
  for( unsigned i=0; i<out_h; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *rop = (oty_t *)(out_yp + out_rofs); \
    oty_t *gop = (oty_t *)(out_up + out_rofs); \
    oty_t *bop = (oty_t *)(out_vp + out_rofs); \

#define xfer_rgb_fltp_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *rip_row = in_yp + in_rofs; \
    uint8_t *gip_row = in_up + in_rofs; \
    uint8_t *bip_row = in_vp + in_rofs; \

#define xfer_rgb_fltp_col_in(ity_t) \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *rip = (ity_t *)(rip_row + in_ofs); \
      ity_t *gip = (ity_t *)(gip_row + in_ofs); \
      ity_t *bip = (ity_t *)(bip_row + in_ofs); \

#define xfer_rgb_floatp_row_out(oty_t) \
  xfer_rgb_fltp_row_out(oty_t) \

#define xfer_rgba_floatp_row_out(oty_t) \
  xfer_rgb_fltp_row_out(oty_t) \
    oty_t *aop = (oty_t *)(out_ap + out_rofs); \

#define xfer_rgb_floatp_row_in(ity_t) \
  xfer_rgb_fltp_row_in(ity_t) \
   xfer_rgb_fltp_col_in(ity_t) \

#define xfer_rgba_floatp_row_in(ity_t) \
  xfer_rgb_fltp_row_in(ity_t) \
    uint8_t *aip_row = in_ap + in_rofs; \
    xfer_rgb_fltp_col_in(ity_t) \
      ity_t *aip = (ity_t *)(aip_row + in_ofs); \


class BC_Xfer {
  BC_Xfer(const BC_Xfer&) {}
public:
  BC_Xfer(uint8_t **output_rows, uint8_t **input_rows,
    uint8_t *out_yp, uint8_t *out_up, uint8_t *out_vp,
    uint8_t *in_yp, uint8_t *in_up, uint8_t *in_vp,
    int in_x, int in_y, int in_w, int in_h, int out_x, int out_y, int out_w, int out_h,
    int in_colormodel, int out_colormodel, int bg_color, int in_rowspan, int out_rowspan);
  BC_Xfer(uint8_t **output_ptrs, int out_colormodel,
      int out_x, int out_y, int out_w, int out_h, int out_rowspan,
    uint8_t **input_ptrs, int in_colormodel,
      int in_x, int in_y, int in_w, int in_h, int in_rowspan,
    int bg_color);
  ~BC_Xfer();

  uint8_t **output_rows, **input_rows;
  uint8_t *out_yp, *out_up, *out_vp, *out_ap;
  uint8_t *in_yp, *in_up, *in_vp, *in_ap;
  int in_x, in_y; unsigned in_w, in_h;
  int out_x, out_y; unsigned out_w, out_h;
  int in_colormodel, out_colormodel;
  uint32_t bg_color, total_in_w, total_out_w;
  int scale;
  int out_pixelsize, in_pixelsize;
  int *row_table, *column_table;
  uint32_t bg_r, bg_g, bg_b;

  void xfer();

  static void init();
  static class Tables { public: Tables() { init(); } } tables;
  static int rtoy_tab[0x100], gtoy_tab[0x100], btoy_tab[0x100];
  static int rtou_tab[0x100], gtou_tab[0x100], btou_tab[0x100];
  static int rtov_tab[0x100], gtov_tab[0x100], btov_tab[0x100];
  static int vtor_tab[0x100], vtog_tab[0x100];
  static int utog_tab[0x100], utob_tab[0x100];
  static float vtor_float_tab[0x100], vtog_float_tab[0x100];
  static float utog_float_tab[0x100], utob_float_tab[0x100];
  static int rtoy_tab16[0x10000], gtoy_tab16[0x10000], btoy_tab16[0x10000];
  static int rtou_tab16[0x10000], gtou_tab16[0x10000], btou_tab16[0x10000];
  static int rtov_tab16[0x10000], gtov_tab16[0x10000], btov_tab16[0x10000];
  static int vtor_tab16[0x10000], vtog_tab16[0x10000];
  static int utog_tab16[0x10000], utob_tab16[0x10000];
  static float v16tor_float_tab[0x10000], v16tog_float_tab[0x10000];
  static float u16tog_float_tab[0x10000], u16tob_float_tab[0x10000];

  void init(
    uint8_t **output_rows, int out_colormodel, int out_x, int out_y, int out_w, int out_h,
      uint8_t *out_yp, uint8_t *out_up, uint8_t *out_vp, uint8_t *out_ap, int out_rowspan,
    uint8_t **input_rows, int in_colormodel, int in_x, int in_y, int in_w, int in_h,
      uint8_t *in_yp, uint8_t *in_up, uint8_t *in_vp, uint8_t *in_ap, int in_rowspan,
    int bg_color);

// generated code concatentated here
