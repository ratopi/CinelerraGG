
// Compression coefficients straight out of jpeglib
#define R_TO_Y    0.29900
#define G_TO_Y    0.58700
#define B_TO_Y    0.11400

#define R_TO_U    -0.16874
#define G_TO_U    -0.33126
#define B_TO_U    0.50000

#define R_TO_V    0.50000
#define G_TO_V    -0.41869
#define B_TO_V    -0.08131

// Decompression coefficients straight out of jpeglib
#define V_TO_R    1.40200
#define V_TO_G    -0.71414

#define U_TO_G    -0.34414
#define U_TO_B    1.77200

int BC_Xfer::rtoy_tab[0x100], BC_Xfer::gtoy_tab[0x100], BC_Xfer::btoy_tab[0x100];
int BC_Xfer::rtou_tab[0x100], BC_Xfer::gtou_tab[0x100], BC_Xfer::btou_tab[0x100];
int BC_Xfer::rtov_tab[0x100], BC_Xfer::gtov_tab[0x100], BC_Xfer::btov_tab[0x100];
int BC_Xfer::vtor_tab[0x100], BC_Xfer::vtog_tab[0x100];
int BC_Xfer::utog_tab[0x100], BC_Xfer::utob_tab[0x100];
float BC_Xfer::vtor_float_tab[0x100], BC_Xfer::vtog_float_tab[0x100];
float BC_Xfer::utog_float_tab[0x100], BC_Xfer::utob_float_tab[0x100];
int BC_Xfer::rtoy_tab16[0x10000], BC_Xfer::gtoy_tab16[0x10000], BC_Xfer::btoy_tab16[0x10000];
int BC_Xfer::rtou_tab16[0x10000], BC_Xfer::gtou_tab16[0x10000], BC_Xfer::btou_tab16[0x10000];
int BC_Xfer::rtov_tab16[0x10000], BC_Xfer::gtov_tab16[0x10000], BC_Xfer::btov_tab16[0x10000];
int BC_Xfer::vtor_tab16[0x10000], BC_Xfer::vtog_tab16[0x10000];
int BC_Xfer::utog_tab16[0x10000], BC_Xfer::utob_tab16[0x10000];
float BC_Xfer::v16tor_float_tab[0x10000], BC_Xfer::v16tog_float_tab[0x10000];
float BC_Xfer::u16tog_float_tab[0x10000], BC_Xfer::u16tob_float_tab[0x10000];

BC_Xfer::Tables BC_Xfer::tables;

void BC_Xfer::init()
{
	for( int i=0; i<0x100; ++i ) {
		rtoy_tab[i] = (int)(R_TO_Y * 0x10000 * i);
		rtou_tab[i] = (int)(R_TO_U * 0x10000 * i);
		rtov_tab[i] = (int)(R_TO_V * 0x10000 * i);

		gtoy_tab[i] = (int)(G_TO_Y * 0x10000 * i);
		gtou_tab[i] = (int)(G_TO_U * 0x10000 * i);
		gtov_tab[i] = (int)(G_TO_V * 0x10000 * i);

		btoy_tab[i] = (int)(B_TO_Y * 0x10000 * i);
		btou_tab[i] = (int)(B_TO_U * 0x10000 * i) + 0x800000;
		btov_tab[i] = (int)(B_TO_V * 0x10000 * i) + 0x800000;
	}

	for( int i=0; i<0x10000; ++i ) {
		rtoy_tab16[i] = (int)(R_TO_Y * 0x100 * i);
		rtou_tab16[i] = (int)(R_TO_U * 0x100 * i);
		rtov_tab16[i] = (int)(R_TO_V * 0x100 * i);

		gtoy_tab16[i] = (int)(G_TO_Y * 0x100 * i);
		gtou_tab16[i] = (int)(G_TO_U * 0x100 * i);
		gtov_tab16[i] = (int)(G_TO_V * 0x100 * i);

		btoy_tab16[i] = (int)(B_TO_Y * 0x100 * i);
		btou_tab16[i] = (int)(B_TO_U * 0x100 * i) + 0x800000;
		btov_tab16[i] = (int)(B_TO_V * 0x100 * i) + 0x800000;
	}

	for( int i=-0x80; i<0x80; ++i ) {
		vtor_tab[i+0x80] = (int)(V_TO_R * 0x10000 * i);
		vtog_tab[i+0x80] = (int)(V_TO_G * 0x10000 * i);
		utog_tab[i+0x80] = (int)(U_TO_G * 0x10000 * i);
		utob_tab[i+0x80] = (int)(U_TO_B * 0x10000 * i);
	}

	for( int i=-0x80; i<0x80; ++i ) {
		vtor_float_tab[i+0x80] = V_TO_R * i / 0xff;
		vtog_float_tab[i+0x80] = V_TO_G * i / 0xff;
		utog_float_tab[i+0x80] = U_TO_G * i / 0xff;
		utob_float_tab[i+0x80] = U_TO_B * i / 0xff;
	}

	for( int i=-0x8000; i<0x8000; ++i ) {
		vtor_tab16[i+0x8000] = (int)(V_TO_R * 0x100 * i);
		vtog_tab16[i+0x8000] = (int)(V_TO_G * 0x100 * i);
		utog_tab16[i+0x8000] = (int)(U_TO_G * 0x100 * i);
		utob_tab16[i+0x8000] = (int)(U_TO_B * 0x100 * i);
	}

	for( int i=-0x8000; i<0x8000; ++i ) {
		v16tor_float_tab[i+0x8000] = V_TO_R * i / 0xffff;
		v16tog_float_tab[i+0x8000] = V_TO_G * i / 0xffff;
		u16tog_float_tab[i+0x8000] = U_TO_G * i / 0xffff;
		u16tob_float_tab[i+0x8000] = U_TO_B * i / 0xffff;
	}
}

void BC_Xfer::init(
	uint8_t **output_rows, int out_colormodel, int out_x, int out_y, int out_w, int out_h,
		uint8_t *out_yp, uint8_t *out_up, uint8_t *out_vp, uint8_t *out_ap, int out_rowspan,
	uint8_t **input_rows, int in_colormodel, int in_x, int in_y, int in_w, int in_h,
		uint8_t *in_yp, uint8_t *in_up, uint8_t *in_vp, uint8_t *in_ap, int in_rowspan,
	int bg_color)
{
	// prevent bounds errors on poorly dimensioned macro pixel formats
	switch( in_colormodel ) {
	case BC_YUV422:   in_w &= ~1;               break;  // 2x1
	case BC_YUV420P:  in_w &= ~1;  in_h &= ~1;  break;  // 2x2
	case BC_YUV422P:  in_w &= ~1;               break;  // 2x1
	case BC_YUV410P:  in_w &= ~3;  in_h &= ~3;  break;  // 4x4
	case BC_YUV411P:  in_w &= ~3;               break;  // 4x1
	}
	switch( out_colormodel ) {
	case BC_YUV422:  out_w &= ~1;               break;
	case BC_YUV420P: out_w &= ~1; out_h &= ~1;  break;
	case BC_YUV422P: out_w &= ~1;               break;
	case BC_YUV410P: out_w &= ~3; out_h &= ~3;  break;
	case BC_YUV411P: out_w &= ~3;               break;
	}
	this->output_rows = output_rows;
	this->input_rows = input_rows;
	this->out_yp = out_yp;
	this->out_up = out_up;
	this->out_vp = out_vp;
	this->out_ap = out_ap;
	this->in_yp = in_yp;
	this->in_up = in_up;
	this->in_vp = in_vp;
	this->in_ap = in_ap;
	this->in_x = in_x;
	this->in_y = in_y;
	this->in_w = in_w;
	this->in_h = in_h;
	this->out_x = out_x;
	this->out_y = out_y;
	this->out_w = out_w;
	this->out_h = out_h;
	this->in_colormodel = in_colormodel;
	switch( in_colormodel ) {
	case BC_RGB_FLOATP:
	case BC_RGBA_FLOATP:
		in_rowspan = in_w * sizeof(float);
		break;
	}
	this->total_in_w = in_rowspan;
	this->out_colormodel = out_colormodel;
	switch( out_colormodel ) {
	case BC_RGB_FLOATP:
	case BC_RGBA_FLOATP:
		out_rowspan = out_w * sizeof(float);
		break;
	}
	this->total_out_w = out_rowspan;
	this->bg_color = bg_color;
	this->bg_r = (bg_color>>16) & 0xff;
	this->bg_g = (bg_color>>8) & 0xff;
	this->bg_b = (bg_color>>0) & 0xff;

	this->in_pixelsize = BC_CModels::calculate_pixelsize(in_colormodel);
	this->out_pixelsize = BC_CModels::calculate_pixelsize(out_colormodel);
	this->scale = (out_w != in_w) || (in_x != 0);

/* + 1 so we don't overflow when calculating in advance */ 
	column_table = new int[out_w+1];
        double hscale = (double)in_w/out_w;
	for( int i=0; i<out_w; ++i ) {
			column_table[i] = ((int)(hscale * i) + in_x) * in_pixelsize;
			if( in_colormodel == BC_YUV422 ) column_table[i] &= ~3;
	}
        double vscale = (double)in_h/out_h;
	row_table = new int[out_h];
        for( int i=0; i<out_h; ++i )
                row_table[i] = (int)(vscale * i) + in_y;
}

BC_Xfer::BC_Xfer(uint8_t **output_rows, uint8_t **input_rows,
	uint8_t *out_yp, uint8_t *out_up, uint8_t *out_vp,
	uint8_t *in_yp, uint8_t *in_up, uint8_t *in_vp,
	int in_x, int in_y, int in_w, int in_h, int out_x, int out_y, int out_w, int out_h,
	int in_colormodel, int out_colormodel, int bg_color, int in_rowspan, int out_rowspan)
{
	init(output_rows, out_colormodel, out_x, out_y, out_w, out_h,
		out_yp, out_up, out_vp, 0, out_rowspan,
	     input_rows, in_colormodel, in_x, in_y, in_w, in_h,
		in_yp, in_up, in_vp, 0, in_rowspan,  bg_color);
}

BC_Xfer::BC_Xfer(
	uint8_t **output_ptrs, int out_colormodel,
		int out_x, int out_y, int out_w, int out_h, int out_rowspan,
	uint8_t **input_ptrs, int in_colormodel,
		int in_x, int in_y, int in_w, int in_h, int in_rowspan,
	int bg_color)
{
	uint8_t *out_yp = 0, *out_up = 0, *out_vp = 0, *out_ap = 0;
	uint8_t *in_yp = 0,  *in_up = 0,  *in_vp = 0,  *in_ap = 0;
	if( BC_CModels::is_planar(in_colormodel) ) {
		in_yp = input_ptrs[0]; in_up = input_ptrs[1]; in_vp = input_ptrs[2];
		if( BC_CModels::has_alpha(in_colormodel) ) in_ap = input_ptrs[3];
	}
	if( BC_CModels::is_planar(out_colormodel) ) {
		out_yp = output_ptrs[0]; out_up = output_ptrs[1]; out_vp = output_ptrs[2];
		if( BC_CModels::has_alpha(out_colormodel) ) out_ap = output_ptrs[3];
	}
	init(output_ptrs, out_colormodel, out_x, out_y, out_w, out_h,
		 out_yp, out_up, out_vp, out_ap, out_rowspan,
	     input_ptrs, in_colormodel, in_x, in_y, in_w, in_h,
		 in_yp, in_up, in_vp, in_ap, in_rowspan,  bg_color);
}

BC_Xfer::~BC_Xfer()
{
	delete [] column_table;
	delete [] row_table;
}

void BC_CModels::transfer(unsigned char **output_rows, unsigned char **input_rows,
        unsigned char *out_yp, unsigned char *out_up, unsigned char *out_vp,
        unsigned char *in_yp, unsigned char *in_up, unsigned char *in_vp,
        int in_x, int in_y, int in_w, int in_h, int out_x, int out_y, int out_w, int out_h,
        int in_colormodel, int out_colormodel, int bg_color, int in_rowspan, int out_rowspan)
{
	BC_Xfer xfer(output_rows, input_rows,
		out_yp, out_up, out_vp, in_yp, in_up, in_vp,
		in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h,
		in_colormodel, out_colormodel, bg_color, in_rowspan, out_rowspan);
	xfer.xfer();
}

void BC_CModels::transfer(
	uint8_t **output_ptrs, int out_colormodel,
		int out_x, int out_y, int out_w, int out_h, int out_rowspan,
	uint8_t **input_ptrs, int in_colormodel,
		int in_x, int in_y, int in_w, int in_h, int in_rowspan,  int bg_color)
{
	BC_Xfer xfer(
		output_ptrs, out_colormodel, out_x, out_y, out_w, out_h, out_rowspan,
		input_ptrs, in_colormodel, in_x, in_y, in_w, in_h, in_rowspan,  bg_color);
	xfer.xfer();
}

// specialized functions

//  in bccmdl.py:  specialize("bc_rgba8888", "bc_transparency", "XFER_rgba8888_to_transparency")
void BC_Xfer::XFER_rgba8888_to_transparency()
{
  for( unsigned i=0; i<out_h; ++i ) {
    uint8_t *outp = output_rows[i + out_y] + out_x * out_pixelsize;
    uint8_t *inp_row = input_rows[row_table[i]];
    int bit_no = 0, bit_vec = 0;
    for( unsigned j=0; j<out_w; ) {
      uint8_t *inp = inp_row + column_table[j];
      if( inp[3] < 127 ) bit_vec |= 0x01 << bit_no;
      bit_no = ++j & 7;
      if( !bit_no ) { *outp++ = bit_vec;  bit_vec = 0; }
    }
    if( bit_no ) *outp = bit_vec;
  } 
}

