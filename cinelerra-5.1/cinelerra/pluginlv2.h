#ifndef __PLUGINLV2_H__
#define __PLUGINLV2_H__

#define LV2_SEQ_SIZE  9624
#include "forkbase.h"
#include "pluginlv2config.h"
#include "samples.inc"

#include <lilv/lilv.h>
#define NS_EXT "http://lv2plug.in/ns/ext/"

typedef struct {
	int64_t sz;
	int samples, done;
	int nb_inputs, nb_outputs;
} shm_bfr_t;

#define TYP_AUDIO   1
#define TYP_CONTROL 2
#define TYP_ATOM    4
#define TYP_ALL    ~0

class PluginLV2
{
public:
	PluginLV2();
	virtual ~PluginLV2();

	shm_bfr_t *shm_bfr;
	int use_shm, shmid;
	float **in_buffers, **out_buffers;
	int *iport, *oport;
	int nb_inputs, nb_outputs;
	int max_bufsz, ui_features;

	void reset_lv2();
	int load_lv2(const char *path,char *title=0);
	int init_lv2(PluginLV2ClientConfig &conf, int sample_rate);

	static LV2_URID uri_table_map(LV2_URID_Map_Handle handle, const char *uri);
	static const char *uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid);
	void connect_ports(PluginLV2ClientConfig &conf, int typ=TYP_ALL);
	void del_buffer();
	void new_buffer(int64_t sz);
	shm_bfr_t *shm_buffer(int shmid);
	void init_buffer(int samples);
	void map_buffer();

	LilvWorld         *world;
	const LilvPlugin  *lilv;
	LilvUIs           *lilv_uis;

	PluginLV2UriTable  uri_table;
	LV2_URID_Map       map;
	LV2_Feature        map_feature;
	LV2_URID_Unmap     unmap;
	LV2_Feature        unmap_feature;
	Lv2Features        features;
	LV2_Atom_Sequence  seq_in[2];
	LV2_Atom_Sequence  *seq_out;

	LilvInstance *inst;
	SuilInstance *sinst;
	SuilHost *ui_host;

	LilvNode *atom_AtomPort;
	LilvNode *atom_Sequence;
	LilvNode *lv2_AudioPort;
	LilvNode *lv2_CVPort;
	LilvNode *lv2_ControlPort;
	LilvNode *lv2_Optional;
	LilvNode *lv2_InputPort;
	LilvNode *lv2_OutputPort;
	LilvNode *powerOf2BlockLength;
	LilvNode *fixedBlockLength;
	LilvNode *boundedBlockLength;
};

typedef struct { int sample_rate;  char path[1]; } open_bfr_t;
typedef struct { int idx;  float value; } control_bfr_t;

enum { NO_COMMAND,
	LV2_OPEN,
	LV2_LOAD,
	LV2_UPDATE,
	LV2_SHOW,
	LV2_HIDE,
	LV2_SET,
	LV2_SHMID,
	NB_COMMANDS };

#endif
