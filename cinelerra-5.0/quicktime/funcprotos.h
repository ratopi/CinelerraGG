#ifndef FUNCPROTOS_H
#define FUNCPROTOS_H

#include "graphics.h"
#include "qtprivate.h"

// Used internally to replace ftello values
int64_t quicktime_byte_position(quicktime_t *file);
int64_t quicktime_ftell(quicktime_t *file);
int quicktime_fseek(quicktime_t *file,int64_t offset);

quicktime_trak_t* quicktime_add_track(quicktime_t *file);



int quicktime_file_open(quicktime_t *file, char *path, int rd, int wr);
int quicktime_file_close(quicktime_t *file);
int64_t quicktime_get_file_length(char *path);

/* Initialize audio_map_t and video_map_t objects after loading headers */
/* Called by quicktime_read_info */
void quicktime_init_maps(quicktime_t *file);

int quicktime_read_char(quicktime_t *file);
float quicktime_read_fixed32(quicktime_t *file);
float quicktime_read_fixed16(quicktime_t *file);
int64_t quicktime_read_int64(quicktime_t *file);
int64_t quicktime_read_int64_le(quicktime_t *file);
unsigned long quicktime_read_uint32(quicktime_t *file);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int32_le(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int64_t quicktime_position(quicktime_t *file);
int quicktime_set_position(quicktime_t *file, int64_t position);
int quicktime_write_fixed32(quicktime_t *file, float number);
int quicktime_write_char(quicktime_t *file, char x);
int quicktime_write_int16(quicktime_t *file, int number);
int quicktime_write_int16_le(quicktime_t *file, int number);
int quicktime_write_int24(quicktime_t *file, long number);
int quicktime_write_int32(quicktime_t *file, long value);
int quicktime_write_int32_le(quicktime_t *file, long value);
int quicktime_write_int64(quicktime_t *file, int64_t value);
int quicktime_write_int64_le(quicktime_t *file, int64_t value);
int quicktime_write_char32(quicktime_t *file, char *string);
int quicktime_write_fixed16(quicktime_t *file, float number);
int quicktime_read_int16(quicktime_t *file);
int quicktime_read_int16_le(quicktime_t *file);
void quicktime_copy_char32(char *output,char *input);
void quicktime_read_char32(quicktime_t *file,char *string);

/* Returns number of bytes written */
int quicktime_write_data(quicktime_t *file, char *data, int size);
/* Enable or disable presave */
void quicktime_set_presave(quicktime_t *file, int value);
/* Returns 1 if equal or 0 if different */
int quicktime_match_32(char *input, char *output);
int quicktime_match_24(char *input, char *output);
void quicktime_print_chars(char *desc,char *input,int len);
void quicktime_write_pascal(quicktime_t *file, char *data);
void quicktime_read_pascal(quicktime_t *file,char *data);
int quicktime_read_data(quicktime_t *file, char *data, int64_t size);
/* Quantize the number to the nearest multiple of 16 */
/* Used by ffmpeg codecs */
int quicktime_quantize2(int number);
int quicktime_quantize8(int number);
int quicktime_quantize16(int number);
int quicktime_quantize32(int number);



/* Convert Microsoft audio id to codec */
void quicktime_id_to_codec(char *result, int id);

int quicktime_find_vcodec(quicktime_video_map_t *vtrack);
int quicktime_find_acodec(quicktime_audio_map_t *atrack);




/* Track functions */


/* Most codecs don't specify the actual number of bits on disk in the stbl. */
/* Convert the samples to the number of bytes for reading depending on the codec. */
int64_t quicktime_samples_to_bytes(quicktime_trak_t *track, long samples);

char* quicktime_compressor(quicktime_trak_t *trak);

/* Get median duration of video frames for calculating frame rate. */
int quicktime_sample_duration(quicktime_trak_t *trak);

/* Graphics */
quicktime_scaletable_t* quicktime_new_scaletable(int input_w, int input_h, int output_w, int output_h);



/* chunks always start on 1 */
/* samples start on 0 */

/* update the position pointers in all the tracks after a set_position */
int quicktime_update_positions(quicktime_t *file);







/* AVI section */
void quicktime_read_strh(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_write_strh(quicktime_t *file, int track);




/* Create new strl object for reading or writing */
quicktime_strl_t* quicktime_new_strl();
/* Initialize new strl object for writing */
void quicktime_init_strl(quicktime_t *file,
	quicktime_audio_map_t *atrack,
	quicktime_video_map_t *vtrack,
	quicktime_trak_t *trak,
	quicktime_strl_t *strl);
void quicktime_delete_strl(quicktime_strl_t *strl);
/* Read strl object */
void quicktime_read_strl(quicktime_t *file,
	quicktime_strl_t *strl,
	quicktime_atom_t *parent_atom);


void quicktime_delete_indx(quicktime_indx_t *indx);
void quicktime_init_indx(quicktime_t *file,
	quicktime_indx_t *indx,
	quicktime_strl_t *strl);
void quicktime_update_indx(quicktime_t *file,
	quicktime_indx_t *indx,
	quicktime_ix_t *ix);
void quicktime_finalize_indx(quicktime_t *file);
/* Called by quicktime_read_strl */
void quicktime_read_indx(quicktime_t *file,
	quicktime_strl_t *strl,
	quicktime_atom_t *parent_atom);



void quicktime_delete_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_read_hdrl(quicktime_t *file,
	quicktime_hdrl_t *hdrl,
	quicktime_atom_t *parent_atom);
void quicktime_init_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_finalize_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);


/* Read movi and create objects */
void quicktime_delete_movi(quicktime_t *file, quicktime_movi_t *movi);
void quicktime_read_movi(quicktime_t *file,
	quicktime_atom_t *parent_atom,
	quicktime_movi_t *movi);
/* Write preliminary movi data and create objects */
void quicktime_init_movi(quicktime_t *file, quicktime_riff_t *riff);
void quicktime_finalize_movi(quicktime_t *file, quicktime_movi_t *movi);






void quicktime_delete_idx1(quicktime_idx1_t *idx1);
void quicktime_read_idx1(quicktime_t *file,
	quicktime_riff_t *riff,
	quicktime_atom_t *parent_atom);
void quicktime_write_idx1(quicktime_t *file,
	quicktime_idx1_t *idx1);
/* Set the keyframe flag in the right IDX1 entry.  new_keyframes starts on 0 */
/* Used by quicktime_insert_keyframe */
void quicktime_set_idx1_keyframe(quicktime_t *file,
	quicktime_trak_t *trak,
	int new_keyframe);
/* Create new IDX1 entry.  Used by quicktime_write_chunk_footer */
void quicktime_update_idx1table(quicktime_t *file,
	quicktime_trak_t *trak,
	int offset,
	int size);




void quicktime_read_odml(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_finalize_odml(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_init_odml(quicktime_t *file, quicktime_hdrl_t *hdrl);








/* Create new riff and put on riff table */
quicktime_riff_t* quicktime_new_riff(quicktime_t *file);
void quicktime_delete_riff(quicktime_t *file, quicktime_riff_t *riff);
void quicktime_read_riff(quicktime_t *file,quicktime_atom_t *parent_atom);

/* Write riff header and initialize structures */
/* Only the first riff has a hdrl object */
void quicktime_init_riff(quicktime_t *file);
void quicktime_finalize_riff(quicktime_t *file, quicktime_riff_t *riff);
void quicktime_import_avi(quicktime_t *file);


/* Create ix object for writing only */
quicktime_ix_t* quicktime_new_ix(quicktime_t *file,
	quicktime_trak_t *trak,
	quicktime_strl_t *strl);
void quicktime_delete_ix(quicktime_ix_t *ix);
void quicktime_update_ixtable(quicktime_t *file,
	quicktime_trak_t *trak,
	int64_t offset,
	int size);
void quicktime_write_ix(quicktime_t *file,
	quicktime_ix_t *ix,
	int track);
/* Read entire ix atom from current position in file */
/* Called by quicktime_read_indx. */
void quicktime_read_ix(quicktime_t *file,
	quicktime_ix_t *ix);

/* the byte offset from mdat start of the chunk.  Chunk is numbered from 1 */
int64_t quicktime_chunk_to_offset(quicktime_t *file, quicktime_trak_t *trak, long chunk);


/* the chunk of any offset from mdat start */
long quicktime_offset_to_chunk(int64_t *chunk_offset,
	quicktime_trak_t *trak, int64_t offset);

/* Bytes in the chunk.  Chunk is numbered from 1 */
/* Only available in AVI. */
/* Reads the chunk header to get the size. */
/* Stores the chunk offset in chunk_offset. */
/* Returns the bytes in the chunk less the 8 byte header size */
int quicktime_chunk_bytes(quicktime_t *file,
	int64_t *chunk_offset, int chunk, quicktime_trak_t *trak);

/* total bytes between the two samples */
int64_t quicktime_sample_range_size(quicktime_trak_t *trak,
    int64_t chunk_sample, int64_t sample);

/* converting between mdat offsets to samples */
int64_t quicktime_sample_to_offset(quicktime_t *file, quicktime_trak_t *trak, int64_t sample);
long quicktime_offset_to_sample(quicktime_trak_t *trak, int64_t offset);


void quicktime_write_chunk_header(quicktime_t *file,
	quicktime_trak_t *trak, quicktime_atom_t *chunk);
void quicktime_write_chunk_footer(quicktime_t *file,
	quicktime_trak_t *trak, int current_chunk,
	quicktime_atom_t *chunk, int samples);

/* Write VBR frame */
int quicktime_write_vbr_frame(quicktime_t *file, int track,
	char *data, int data_size, int samples);
/* update all the tables after writing a buffer */
/* set sample_size to 0 if no sample size should be set */
int quicktime_trak_duration(quicktime_trak_t *trak, long *duration, long *timescale);
int quicktime_trak_fix_counts(quicktime_t *file, quicktime_trak_t *trak);
int quicktime_sample_size(quicktime_trak_t *trak, int sample);

/* number of samples in the chunk */
long quicktime_chunk_samples(quicktime_trak_t *trak, long chunk);
int quicktime_trak_shift_offsets(quicktime_trak_t *trak, int64_t offset);

int quicktime_get_timescale(double frame_rate);
unsigned long quicktime_current_time(void);


// Utility functions for decoding vbr audio codecs

// init/delete buffers
void quicktime_init_vbr(quicktime_vbr_t *vbr,int channels);
void quicktime_clear_vbr(quicktime_vbr_t *vbr);
// next sample time at end of input/output buffer
int64_t quicktime_vbr_input_end(quicktime_vbr_t *vbr);
int64_t quicktime_vbr_output_end(quicktime_vbr_t *vbr);
// next input byte at input_buffer[inp_ptr]
unsigned char *quicktime_vbr_input(quicktime_vbr_t *vbr);
// bytes in quicktime_vbr_input data
int quicktime_vbr_input_size(quicktime_vbr_t *vbr);
// check max xfer size bounds
int quicktime_limit_samples(int samples);
// seek frame of sample_time, then backup offset frames
int quicktime_seek_vbr(quicktime_audio_map_t *atrack, int64_t start_time, int offset);
// shift input buffer down inp_ptr bytes
int quicktime_align_vbr(quicktime_audio_map_t *atrack, int64_t start_position);
// Append the next sample/frame of compressed data to the input buffer
int quicktime_read_vbr(quicktime_t *file,quicktime_audio_map_t *atrack);
// Shift the input buffer by adjusting inp_ptr += bytes
void quicktime_shift_vbr(quicktime_audio_map_t *atrack, int bytes);
// Put the frame of sample data in output and advance the counters
void quicktime_store_vbr_float(quicktime_audio_map_t *atrack,
   float *samples, int sample_count);
void quicktime_store_vbr_floatp(quicktime_audio_map_t *atrack,
   float **samples, int sample_count);
void quicktime_store_vbr_int16(quicktime_audio_map_t *atrack,
   int16_t *samples, int sample_count);
// get the frame of sample data from output ring buffer
void quicktime_copy_vbr_float(quicktime_vbr_t *vbr,
   int64_t start_position, int sample_count,float *output,int channel);
void quicktime_copy_vbr_int16(quicktime_vbr_t *vbr,
   int64_t start_position, int samples,int16_t *output,int channel);

// Frame caching for keyframed video.
quicktime_cache_t* quicktime_new_cache();
void quicktime_cache_max(quicktime_cache_t *ptr, int bytes);
void quicktime_delete_cache(quicktime_cache_t *ptr);
void quicktime_reset_cache(quicktime_cache_t *ptr);
void quicktime_put_frame(quicktime_cache_t *ptr, int64_t frame_number,
	unsigned char *y, unsigned char *u, unsigned char *v,
	int y_size, int u_size, int v_size);
// Return 1 if the frame was found.
int quicktime_get_frame(quicktime_cache_t *ptr, int64_t frame_number,
	unsigned char **y, unsigned char **u, unsigned char **v);
int quicktime_has_frame(quicktime_cache_t *ptr,
	int64_t frame_number);
int64_t quicktime_cache_usage(quicktime_cache_t *ptr);

/* matrix.c */
void quicktime_matrix_init(quicktime_matrix_t *matrix);
void quicktime_matrix_delete(quicktime_matrix_t *matrix);
void quicktime_read_matrix(quicktime_t *file,quicktime_matrix_t *matrix);
void quicktime_matrix_dump(quicktime_matrix_t *matrix);
void quicktime_write_matrix(quicktime_t *file,quicktime_matrix_t *matrix);

/* atom.c */
int quicktime_atom_read_header(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_atom_write_header64(quicktime_t *file,quicktime_atom_t *atom,
   char *text);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,
   char *text);
void quicktime_atom_write_footer(quicktime_t *file, quicktime_atom_t *atom);
int quicktime_atom_is(quicktime_atom_t *atom, char *type);
int quicktime_atom_skip(quicktime_t *file, quicktime_atom_t *atom);

/* ctab.c */
int quicktime_ctab_init(quicktime_ctab_t *ctab);
int quicktime_ctab_delete(quicktime_ctab_t *ctab);
void quicktime_ctab_dump(quicktime_ctab_t *ctab);
int quicktime_read_ctab(quicktime_t *file,quicktime_ctab_t *ctab);

/* dref.c */
void quicktime_dref_table_init(quicktime_dref_table_t *table);
void quicktime_dref_table_delete(quicktime_dref_table_t *table);
void quicktime_read_dref_table(quicktime_t *file,quicktime_dref_table_t *table);
void quicktime_write_dref_table(quicktime_t *file,quicktime_dref_table_t *table);
void quicktime_dref_table_dump(quicktime_dref_table_t *table);
void quicktime_dref_init(quicktime_dref_t *dref);
void quicktime_dref_init_all(quicktime_dref_t *dref);
void quicktime_dref_delete(quicktime_dref_t *dref);
void quicktime_dref_dump(quicktime_dref_t *dref);
void quicktime_read_dref(quicktime_t *file,quicktime_dref_t *dref);
void quicktime_write_dref(quicktime_t *file,quicktime_dref_t *dref);

/* avcc.c */
void quicktime_delete_avcc(quicktime_avcc_t *avcc);
void quicktime_set_avcc_header(quicktime_avcc_t *avcc,unsigned char *data,int size);
void quicktime_write_avcc(quicktime_t *file,quicktime_avcc_t *avcc);
int quicktime_read_avcc(quicktime_t *file,quicktime_atom_t *parent_atom,
   quicktime_avcc_t *avcc);
void quicktime_avcc_dump(quicktime_avcc_t *avcc);

/* esds.c */
void quicktime_delete_esds(quicktime_esds_t *esds);
/* Get alternative mpeg4 parameters from esds table */
void quicktime_esds_samplerate(quicktime_stsd_table_t *table,
   quicktime_esds_t *esds);
void quicktime_read_esds(quicktime_t *file,quicktime_atom_t *parent_atom,
   quicktime_esds_t *esds);
void quicktime_write_esds(quicktime_t *file,quicktime_esds_t *esds,int do_video
   ,int do_audio);
void quicktime_esds_dump(quicktime_esds_t *esds);

/* frma.c */
void quicktime_delete_frma(quicktime_frma_t *frma);
int quicktime_read_frma(quicktime_t *file,quicktime_atom_t *parent_atom,
   quicktime_atom_t *leaf_atom,quicktime_frma_t *frma);
void quicktime_frma_dump(quicktime_frma_t *frma);

/* hdlr.c */
void quicktime_hdlr_init(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_init_video(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_init_audio(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_init_data(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_delete(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_dump(quicktime_hdlr_t *hdlr);
void quicktime_read_hdlr(quicktime_t *file,quicktime_hdlr_t *hdlr);
void quicktime_write_hdlr(quicktime_t *file,quicktime_hdlr_t *hdlr);

/* avi_hdrl.c */
void quicktime_delete_hdrl(quicktime_t *file,quicktime_hdrl_t *hdrl);
void quicktime_read_hdrl(quicktime_t *file,quicktime_hdrl_t *hdrl,
   quicktime_atom_t *parent_atom);
void quicktime_init_hdrl(quicktime_t *file,quicktime_hdrl_t *hdrl);
void quicktime_finalize_hdrl(quicktime_t *file,quicktime_hdrl_t *hdrl);
/* elst.c */
void quicktime_elst_table_init(quicktime_elst_table_t *table);
void quicktime_elst_table_delete(quicktime_elst_table_t *table);
void quicktime_read_elst_table(quicktime_t *file,quicktime_elst_table_t *table);
void quicktime_write_elst_table(quicktime_t *file,quicktime_elst_table_t *table,
   long duration);
void quicktime_elst_table_dump(quicktime_elst_table_t *table);
void quicktime_elst_init(quicktime_elst_t *elst);
void quicktime_elst_init_all(quicktime_elst_t *elst);
void quicktime_elst_delete(quicktime_elst_t *elst);
void quicktime_elst_dump(quicktime_elst_t *elst);
void quicktime_read_elst(quicktime_t *file,quicktime_elst_t *elst);
void quicktime_write_elst(quicktime_t *file,quicktime_elst_t *elst,
   long duration);

/* mdhd.c */
void quicktime_mdhd_init(quicktime_mdhd_t *mdhd);
void quicktime_mdhd_init_video(quicktime_t *file,quicktime_mdhd_t *mdhd,
   int frame_w,int frame_h,float frame_rate);
void quicktime_mdhd_init_audio(quicktime_mdhd_t *mdhd,int sample_rate);
void quicktime_mdhd_delete(quicktime_mdhd_t *mdhd);
void quicktime_read_mdhd(quicktime_t *file,quicktime_mdhd_t *mdhd);
void quicktime_mdhd_dump(quicktime_mdhd_t *mdhd);
void quicktime_write_mdhd(quicktime_t *file,quicktime_mdhd_t *mdhd);

/* minf.c */
void quicktime_minf_init(quicktime_minf_t *minf);
void quicktime_minf_init_video(quicktime_t *file,quicktime_minf_t *minf,
   int frame_w,int frame_h,int time_scale,float frame_rate,char *compressor);
void quicktime_minf_init_audio(quicktime_t *file,quicktime_minf_t *minf,
   int channels,int sample_rate,int bits,char *compressor);
void quicktime_minf_delete(quicktime_minf_t *minf);
void quicktime_minf_dump(quicktime_minf_t *minf);
int quicktime_read_minf(quicktime_t *file,quicktime_minf_t *minf,
   quicktime_atom_t *parent_atom);
void quicktime_write_minf(quicktime_t *file,quicktime_minf_t *minf);

/* stsd.c */
void quicktime_stsd_init(quicktime_stsd_t *stsd);
void quicktime_stsd_init_table(quicktime_stsd_t *stsd);
void quicktime_stsd_init_video(quicktime_t *file,quicktime_stsd_t *stsd,
   int frame_w,int frame_h,float frame_rate,char *compression);
void quicktime_stsd_init_audio(quicktime_t *file,quicktime_stsd_t *stsd,
   int channels,int sample_rate,int bits,char *compressor);
void quicktime_stsd_delete(quicktime_stsd_t *stsd);
void quicktime_stsd_dump(void *minf_ptr,quicktime_stsd_t *stsd);
void quicktime_read_stsd(quicktime_t *file,quicktime_minf_t *minf,
   quicktime_stsd_t *stsd);
void quicktime_write_stsd(quicktime_t *file,quicktime_minf_t *minf,
   quicktime_stsd_t *stsd);

/* stsdtable.c */
void quicktime_mjqt_init(quicktime_mjqt_t *mjqt);
void quicktime_mjqt_delete(quicktime_mjqt_t *mjqt);
void quicktime_mjqt_dump(quicktime_mjqt_t *mjqt);
void quicktime_mjht_init(quicktime_mjht_t *mjht);
void quicktime_mjht_delete(quicktime_mjht_t *mjht);
void quicktime_mjht_dump(quicktime_mjht_t *mjht);
void quicktime_set_mpeg4_header(quicktime_stsd_table_t *table,
   unsigned char *data,int size);
void quicktime_read_stsd_audio(quicktime_t *file,quicktime_stsd_table_t *table,
   quicktime_atom_t *parent_atom);
void quicktime_write_stsd_audio(quicktime_t *file,quicktime_stsd_table_t *table);
void quicktime_read_stsd_video(quicktime_t *file,quicktime_stsd_table_t *table,
   quicktime_atom_t *parent_atom);
void quicktime_write_stsd_video(quicktime_t *file,quicktime_stsd_table_t *table);
void quicktime_read_stsd_table(quicktime_t *file,quicktime_minf_t *minf,
   quicktime_stsd_table_t *table);
void quicktime_stsd_table_init(quicktime_stsd_table_t *table);
void quicktime_stsd_table_delete(quicktime_stsd_table_t *table);
void quicktime_stsd_video_dump(quicktime_stsd_table_t *table);
void quicktime_stsd_audio_dump(quicktime_stsd_table_t *table);
void quicktime_stsd_table_dump(void *minf_ptr,quicktime_stsd_table_t *table);
void quicktime_write_stsd_table(quicktime_t *file,quicktime_minf_t *minf,
   quicktime_stsd_table_t *table);

/* stts.c */
void quicktime_stts_init(quicktime_stts_t *stts);
void quicktime_stts_init_table(quicktime_stts_t *stts);
void quicktime_stts_init_video(quicktime_t *file,quicktime_stts_t *stts,
   int time_scale,float frame_rate);
void quicktime_stts_init_audio(quicktime_t *file,quicktime_stts_t *stts,
   int sample_rate);
void quicktime_stts_append_audio(quicktime_t *file,quicktime_stts_t *stts,
   int sample_duration);
int64_t quicktime_stts_total_samples(quicktime_t *file,quicktime_stts_t *stts);
void quicktime_stts_delete(quicktime_stts_t *stts);
void quicktime_stts_dump(quicktime_stts_t *stts);
void quicktime_read_stts(quicktime_t *file,quicktime_stts_t *stts);
void quicktime_write_stts(quicktime_t *file,quicktime_stts_t *stts);
int quicktime_time_to_sample(quicktime_stts_t *stts,int64_t *start_time);
int64_t quicktime_chunk_to_samples(quicktime_stts_t *stts, long chunk);

/* stbl.c */
void quicktime_stbl_init(quicktime_stbl_t *stbl);
void quicktime_stbl_init_video(quicktime_t *file,quicktime_stbl_t *stbl,
   int frame_w,int frame_h,int time_scale,float frame_rate,char *compressor);
void quicktime_stbl_init_audio(quicktime_t *file,quicktime_stbl_t *stbl,
   int channels,int sample_rate,int bits,char *compressor);
void quicktime_stbl_delete(quicktime_stbl_t *stbl);
void quicktime_stbl_dump(void *minf_ptr,quicktime_stbl_t *stbl);
int quicktime_read_stbl(quicktime_t *file,quicktime_minf_t *minf,
   quicktime_stbl_t *stbl,quicktime_atom_t *parent_atom);
void quicktime_write_stbl(quicktime_t *file,quicktime_minf_t *minf,
   quicktime_stbl_t *stbl);

/* tkhd.c */
int quicktime_tkhd_init(quicktime_tkhd_t *tkhd);
int quicktime_tkhd_delete(quicktime_tkhd_t *tkhd);
void quicktime_tkhd_dump(quicktime_tkhd_t *tkhd);
void quicktime_read_tkhd(quicktime_t *file,quicktime_tkhd_t *tkhd);
void quicktime_write_tkhd(quicktime_t *file,quicktime_tkhd_t *tkhd);
void quicktime_tkhd_init_video(quicktime_t *file,quicktime_tkhd_t *tkhd,
   int frame_w,int frame_h);

/* stco.c */
void quicktime_stco_init(quicktime_stco_t *stco);
void quicktime_stco_delete(quicktime_stco_t *stco);
void quicktime_stco_init_common(quicktime_t *file,quicktime_stco_t *stco);
void quicktime_stco_dump(quicktime_stco_t *stco);
void quicktime_read_stco(quicktime_t *file,quicktime_stco_t *stco);
void quicktime_read_stco64(quicktime_t *file,quicktime_stco_t *stco);
void quicktime_write_stco(quicktime_t *file,quicktime_stco_t *stco);
void quicktime_update_stco(quicktime_stco_t *stco, long chunk,int64_t offset);

/* stsz.c */
void quicktime_stsz_init(quicktime_stsz_t *stsz);
void quicktime_stsz_init_video(quicktime_t *file,quicktime_stsz_t *stsz);
void quicktime_stsz_init_audio(quicktime_t *file,quicktime_stsz_t *stsz,
   int channels,int bits,char *compressor);
void quicktime_stsz_delete(quicktime_stsz_t *stsz);
void quicktime_stsz_dump(quicktime_stsz_t *stsz);
void quicktime_read_stsz(quicktime_t *file,quicktime_stsz_t *stsz);
void quicktime_write_stsz(quicktime_t *file,quicktime_stsz_t *stsz);
void quicktime_update_stsz(quicktime_stsz_t *stsz, long sample,long sample_size);
int quicktime_sample_size(quicktime_trak_t *trak,int sample);

/* smhd.c */
void quicktime_smhd_init(quicktime_smhd_t *smhd);
void quicktime_smhd_delete(quicktime_smhd_t *smhd);
void quicktime_smhd_dump(quicktime_smhd_t *smhd);
void quicktime_read_smhd(quicktime_t *file,quicktime_smhd_t *smhd);
void quicktime_write_smhd(quicktime_t *file,quicktime_smhd_t *smhd);

/* mdat.c */
void quicktime_mdat_delete(quicktime_mdat_t *mdat);
void quicktime_read_mdat(quicktime_t *file,quicktime_mdat_t *mdat,
   quicktime_atom_t *parent_atom);

/* trak.c */
int quicktime_trak_init(quicktime_trak_t *trak);
int quicktime_trak_init_video(quicktime_t *file,quicktime_trak_t *trak,
   int frame_w,int frame_h,float frame_rate,char *compressor);
int quicktime_trak_init_audio(quicktime_t *file,quicktime_trak_t *trak,
   int channels,int sample_rate,int bits,char *compressor);
int quicktime_trak_delete(quicktime_trak_t *trak);
int quicktime_trak_dump(quicktime_trak_t *trak);
quicktime_trak_t *quicktime_add_trak(quicktime_t *file);
int quicktime_delete_trak(quicktime_moov_t *moov);
int quicktime_read_trak(quicktime_t *file,quicktime_trak_t *trak,
   quicktime_atom_t *trak_atom);
int quicktime_write_trak(quicktime_t *file,quicktime_trak_t *trak,
   long moov_time_scale);
int64_t quicktime_track_end(quicktime_trak_t *trak);
long quicktime_track_samples(quicktime_t *file,quicktime_trak_t *trak);
long quicktime_sample_of_chunk(quicktime_trak_t *trak,long chunk);
int quicktime_avg_chunk_samples(quicktime_t *file, quicktime_trak_t *trak);
int quicktime_chunk_of_sample(int64_t *chunk_sample,int64_t *chunk,
   quicktime_trak_t *trak,int64_t sample);

/* stsc.c */
void quicktime_stsc_init(quicktime_stsc_t *stsc);
void quicktime_stsc_init_table(quicktime_t *file,quicktime_stsc_t *stsc);
void quicktime_stsc_init_video(quicktime_t *file,quicktime_stsc_t *stsc);
void quicktime_stsc_init_audio(quicktime_t *file,quicktime_stsc_t *stsc,
   int sample_rate);
void quicktime_stsc_delete(quicktime_stsc_t *stsc);
void quicktime_stsc_dump(quicktime_stsc_t *stsc);
void quicktime_read_stsc(quicktime_t *file,quicktime_stsc_t *stsc);
void quicktime_write_stsc(quicktime_t *file,quicktime_stsc_t *stsc);
int quicktime_update_stsc(quicktime_stsc_t *stsc,long chunk,long samples);

/* dinf.c */
void quicktime_dinf_init(quicktime_dinf_t *dinf);
void quicktime_dinf_delete(quicktime_dinf_t *dinf);
void quicktime_dinf_init_all(quicktime_dinf_t *dinf);
void quicktime_dinf_dump(quicktime_dinf_t *dinf);
void quicktime_read_dinf(quicktime_t *file,quicktime_dinf_t *dinf,
   quicktime_atom_t *dinf_atom);
void quicktime_write_dinf(quicktime_t *file,quicktime_dinf_t *dinf);

/* vmhd.c */
void quicktime_vmhd_init(quicktime_vmhd_t *vmhd);
void quicktime_vmhd_init_video(quicktime_t *file,quicktime_vmhd_t *vmhd,
   int frame_w,int frame_h,float frame_rate);
void quicktime_vmhd_delete(quicktime_vmhd_t *vmhd);
void quicktime_vmhd_dump(quicktime_vmhd_t *vmhd);
void quicktime_read_vmhd(quicktime_t *file,quicktime_vmhd_t *vmhd);
void quicktime_write_vmhd(quicktime_t *file,quicktime_vmhd_t *vmhd);

/* mvhd.c */
int quicktime_mvhd_init(quicktime_mvhd_t *mvhd);
int quicktime_mvhd_delete(quicktime_mvhd_t *mvhd);
void quicktime_mvhd_dump(quicktime_mvhd_t *mvhd);
void quicktime_read_mvhd(quicktime_t *file,quicktime_mvhd_t *mvhd,
   quicktime_atom_t *parent_atom);
void quicktime_mhvd_init_video(quicktime_t *file,quicktime_mvhd_t *mvhd,
   double frame_rate);
void quicktime_write_mvhd(quicktime_t *file,quicktime_mvhd_t *mvhd);

/* udta.c */
int quicktime_udta_init(quicktime_udta_t *udta);
int quicktime_udta_delete(quicktime_udta_t *udta);
void quicktime_udta_dump(quicktime_udta_t *udta);
int quicktime_read_udta(quicktime_t *file,quicktime_udta_t *udta,
   quicktime_atom_t *udta_atom);
void quicktime_write_udta(quicktime_t *file,quicktime_udta_t *udta);
int quicktime_read_udta_string(quicktime_t *file,char **string,int *size);
int quicktime_write_udta_string(quicktime_t *file,char *string,int size);
int quicktime_set_udta_string(char **string,int *size,char *new_string);

/* stss.c */
void quicktime_stss_init(quicktime_stss_t *stss);
void quicktime_stss_delete(quicktime_stss_t *stss);
void quicktime_stss_dump(quicktime_stss_t *stss);
void quicktime_read_stss(quicktime_t *file,quicktime_stss_t *stss);
void quicktime_write_stss(quicktime_t *file,quicktime_stss_t *stss);

/* mdia.c */
void quicktime_mdia_init(quicktime_mdia_t *mdia);
void quicktime_mdia_init_video(quicktime_t *file,quicktime_mdia_t *mdia,
   int frame_w,int frame_h,float frame_rate,char *compressor);
void quicktime_mdia_init_audio(quicktime_t *file,quicktime_mdia_t *mdia,
   int channels,int sample_rate,int bits,char *compressor);
void quicktime_mdia_delete(quicktime_mdia_t *mdia);
void quicktime_mdia_dump(quicktime_mdia_t *mdia);
int quicktime_read_mdia(quicktime_t *file,quicktime_mdia_t *mdia,
   quicktime_atom_t *trak_atom);
void quicktime_write_mdia(quicktime_t *file,quicktime_mdia_t *mdia);

/* edts.c */
void quicktime_edts_init(quicktime_edts_t *edts);
void quicktime_edts_delete(quicktime_edts_t *edts);
void quicktime_edts_init_table(quicktime_edts_t *edts);
void quicktime_read_edts(quicktime_t *file,quicktime_edts_t *edts,
   quicktime_atom_t *edts_atom);
void quicktime_edts_dump(quicktime_edts_t *edts);
void quicktime_write_edts(quicktime_t *file,quicktime_edts_t *edts,
   long duration);

/* moov.c */
int quicktime_moov_init(quicktime_moov_t *moov);
int quicktime_moov_delete(quicktime_moov_t *moov);
void quicktime_moov_dump(quicktime_moov_t *moov);
int quicktime_read_moov(quicktime_t *file,quicktime_moov_t *moov,
   quicktime_atom_t *parent_atom);
void quicktime_write_moov(quicktime_t *file,quicktime_moov_t *moov,int rewind);
void quicktime_update_durations(quicktime_moov_t *moov);
int quicktime_shift_offsets(quicktime_moov_t *moov,int64_t offset);

#endif

