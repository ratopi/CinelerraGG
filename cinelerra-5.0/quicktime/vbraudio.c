// Utility functions for vbr audio

#include "funcprotos.h"
#include "quicktime.h"



// Maximum samples to store in output buffer
#define MAX_VBR_BUFFER 0x200000



void quicktime_init_vbr(quicktime_vbr_t *vbr, int channels)
{
	int i;
	vbr->channels = channels;
	vbr->output_buffer = calloc(channels, sizeof(double*));
	for(i = 0; i < channels; i++)
		vbr->output_buffer[i] = calloc(MAX_VBR_BUFFER, sizeof(double));
}

void quicktime_clear_vbr(quicktime_vbr_t *vbr)
{
	if( vbr->output_buffer ) {
		int i;
		for( i=0; i<vbr->channels; ++i )
			free(vbr->output_buffer[i]);
		free(vbr->output_buffer);
	}

	if( vbr->input_buffer )
		free(vbr->input_buffer);
}

int64_t quicktime_vbr_input_end(quicktime_vbr_t *vbr)
{
	return vbr->input_end;
}

int64_t quicktime_vbr_output_end(quicktime_vbr_t *vbr)
{
	return vbr->output_end;
}

unsigned char* quicktime_vbr_input(quicktime_vbr_t *vbr)
{
	return vbr->input_buffer + vbr->inp_ptr;
}

int quicktime_vbr_input_size(quicktime_vbr_t *vbr)
{
	return vbr->input_size;
}

int quicktime_limit_samples(int samples)
{
	if( samples > MAX_VBR_BUFFER ) {
		fprintf(stderr, "quicktime_limit_samples: "
			"can't decode more than 0x%x samples at a time.\n",
			MAX_VBR_BUFFER);
		return 1;
	}
	return 0;
}

int quicktime_seek_vbr(quicktime_audio_map_t *atrack, int64_t start_time, int offset)
{
	quicktime_vbr_t *vbr = &atrack->vbr;
	quicktime_stts_t *stts = &atrack->track->mdia.minf.stbl.stts;
	int sample = quicktime_time_to_sample(stts, &start_time);
	if( (sample-=offset) < 0 ) sample = 0;
	start_time = quicktime_chunk_to_samples(stts, sample);
	vbr->output_end = vbr->input_end = start_time;
	vbr->output_size = vbr->input_size = 0;
	vbr->out_ptr = vbr->inp_ptr = 0;
	vbr->sample = sample;
	return 0;
}

int quicktime_align_vbr(quicktime_audio_map_t *atrack, int64_t start_position)
{
	quicktime_vbr_t *vbr = &atrack->vbr;
	if( start_position < vbr->output_end - vbr->output_size ||
	    start_position > vbr->output_end ) return 1;

	if( vbr->inp_ptr > 0 ) {
		unsigned char *dp = vbr->input_buffer;
		unsigned char *sp = dp + vbr->inp_ptr;
		int i = vbr->input_size;
		while( --i >= 0 ) *dp++ = *sp++;
		vbr->inp_ptr = 0;
	}
	return 0;
}

int quicktime_read_vbr(quicktime_t *file, quicktime_audio_map_t *atrack)
{
	char *input_ptr;
	int result = 0;
	quicktime_vbr_t *vbr = &atrack->vbr;
	quicktime_trak_t *trak = atrack->track;
        quicktime_stts_t *stts = &trak->mdia.minf.stbl.stts;
	int64_t offset = quicktime_sample_to_offset(file, trak, vbr->sample);
	int size = quicktime_sample_size(trak, vbr->sample);
	int new_allocation = vbr->input_size + size;

	if( vbr->input_allocation < new_allocation ) {
		vbr->input_buffer = realloc(vbr->input_buffer, new_allocation);
		vbr->input_allocation = new_allocation;
	}

	quicktime_set_position(file, offset);
	input_ptr = (char *)vbr->input_buffer + vbr->inp_ptr + vbr->input_size;
	result = !quicktime_read_data(file, input_ptr, size);
	vbr->input_size += size;
        vbr->input_end = quicktime_chunk_to_samples(stts, ++vbr->sample);
	return result;
}

void quicktime_shift_vbr(quicktime_audio_map_t *atrack, int bytes)
{
	quicktime_vbr_t *vbr = &atrack->vbr;
	if( bytes >= vbr->input_size ) {
		vbr->inp_ptr = 0;
		vbr->input_size = 0;
	}
	else {
		vbr->inp_ptr += bytes;
		vbr->input_size -= bytes;
	}
}

void quicktime_store_vbr_float(quicktime_audio_map_t *atrack,
	float *samples, int sample_count)
{
	int i, j, k;
	quicktime_vbr_t *vbr = &atrack->vbr;
	int channels = vbr->channels;
	i = vbr->out_ptr;

	for( k=0; k<sample_count; ++k ) {
		for( j=0; j<channels; ++j )
			vbr->output_buffer[j][i] = *samples++;
		if( ++i >= MAX_VBR_BUFFER ) i = 0;
	}

	vbr->out_ptr = i;
	vbr->output_end += sample_count;
	if( (vbr->output_size += sample_count) > MAX_VBR_BUFFER )
		vbr->output_size = MAX_VBR_BUFFER;
}

void quicktime_store_vbr_floatp(quicktime_audio_map_t *atrack,
	float **samples, int sample_count)
{
	int i, j, k;
	quicktime_vbr_t *vbr = &atrack->vbr;
	int channels = vbr->channels;
	i = vbr->out_ptr;

	for( k=0; k<sample_count; ++k ) {
		for( j=0; j<channels; ++j )
			vbr->output_buffer[j][i] = samples[j][k];
		if( ++i >= MAX_VBR_BUFFER ) i = 0;
	}

	vbr->out_ptr = i;
	vbr->output_end += sample_count;
	if( (vbr->output_size += sample_count) > MAX_VBR_BUFFER )
		vbr->output_size = MAX_VBR_BUFFER;
}

void quicktime_store_vbr_int16(quicktime_audio_map_t *atrack,
	int16_t *samples, int sample_count)
{
	int i, j, k;
	quicktime_vbr_t *vbr = &atrack->vbr;
	int channels = vbr->channels;
	i = vbr->out_ptr;

	for( k=0; k<sample_count; ++k ) {
		for( j=0; j<channels; ++j )
			vbr->output_buffer[j][i] = *samples++ / 32768.0;
		if( ++i >= MAX_VBR_BUFFER ) i = 0;
	}

	vbr->out_ptr = i;
	vbr->output_end += sample_count;
	if( (vbr->output_size += sample_count) > MAX_VBR_BUFFER )
		vbr->output_size = MAX_VBR_BUFFER;
}

void quicktime_copy_vbr_float(quicktime_vbr_t *vbr,
	int64_t start_position, int sample_count, float *output, int channel)
{
	int i, k;
	double *sp = vbr->output_buffer[channel];

	i = vbr->out_ptr - (vbr->output_end - start_position);
	while( i < 0 ) i += MAX_VBR_BUFFER;

	for( k=0; k<sample_count; ++k ) {
		*output++ = sp[i++];
		if( i >= MAX_VBR_BUFFER ) i = 0;
	}
}


void quicktime_copy_vbr_int16(quicktime_vbr_t *vbr,
	int64_t start_position, int sample_count, int16_t *output, int channel)
{
	int i, k;
	double *sp = vbr->output_buffer[channel];

	i = vbr->out_ptr - (vbr->output_end - start_position);
	while( i < 0 ) i += MAX_VBR_BUFFER;

	for( k=0; k<sample_count; ++k ) {
		*output++ = sp[i++] * 32767;
		if( i >= MAX_VBR_BUFFER ) i = 0;
	}
}

