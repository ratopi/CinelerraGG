#ifdef HAVE_LV2

#include "bctrace.h"
#include "bcwindowbase.h"
#include "pluginlv2.h"
#include "samples.h"

#include <sys/shm.h>
#include <sys/mman.h>

PluginLV2::PluginLV2()
{
	shm_bfr = 0;
	use_shm = 1;
	shmid = -1;
	in_buffers = 0;   iport = 0;
	out_buffers = 0;  oport = 0;
	nb_inputs = 0;
	nb_outputs = 0;
	max_bufsz = 0;
	ui_features = 0;

	world = 0;
	lilv = 0;
	lilv_uis = 0;
	inst = 0;
	sinst = 0;
	ui_host = 0;

	lv2_InputPort = 0;
	lv2_OutputPort = 0;
	lv2_AudioPort = 0;
	lv2_ControlPort = 0;
	lv2_CVPort = 0;
	lv2_Optional = 0;
	atom_AtomPort = 0;
	atom_Sequence = 0;
	powerOf2BlockLength = 0;
	fixedBlockLength = 0;
	boundedBlockLength = 0;
	seq_out = 0;
}

PluginLV2::~PluginLV2()
{
	reset_lv2();
	if( world )     lilv_world_free(world);
}

void PluginLV2::reset_lv2()
{
	if( inst ) lilv_instance_deactivate(inst);
	lilv_instance_free(inst);             inst = 0;
	lilv_uis_free(lilv_uis);              lilv_uis = 0;

	lilv_node_free(lv2_InputPort);        lv2_InputPort = 0;
	lilv_node_free(lv2_OutputPort);       lv2_OutputPort = 0;
	lilv_node_free(lv2_AudioPort);        lv2_AudioPort = 0;
	lilv_node_free(lv2_ControlPort);      lv2_ControlPort = 0;
	lilv_node_free(lv2_CVPort);           lv2_CVPort = 0;

	lilv_node_free(lv2_Optional);         lv2_Optional = 0;
	lilv_node_free(atom_AtomPort);        atom_AtomPort = 0;
	lilv_node_free(atom_Sequence);        atom_Sequence = 0;
	lilv_node_free(boundedBlockLength);   boundedBlockLength = 0;
	lilv_node_free(fixedBlockLength);     fixedBlockLength = 0;
	lilv_node_free(powerOf2BlockLength);  powerOf2BlockLength = 0;

	delete [] (char *)seq_out;            seq_out = 0;
	uri_table.remove_all_objects();
	features.remove_all_objects();        ui_features = 0;
	del_buffer();
}


int PluginLV2::load_lv2(const char *path, char *title)
{
	if( !world ) {
		world = lilv_world_new();
		if( !world ) {
			printf("lv2: lilv_world_new failed\n");
			return 1;
		}
		lilv_world_load_all(world);
	}

	LilvNode *uri = lilv_new_uri(world, path);
	if( !uri ) {
		printf("lv2: lilv_new_uri(%s) failed\n", path);
		return 1;
	}

	const LilvPlugins *all_plugins = lilv_world_get_all_plugins(world);
	lilv = lilv_plugins_get_by_uri(all_plugins, uri);
	lilv_node_free(uri);
	if( !lilv ) {
		printf("lv2: lilv_plugins_get_by_uriPlugin(%s) failed\n", path);
		return 1;
	}

	if( title ) {
		LilvNode *name = lilv_plugin_get_name(lilv);
		const char *nm = lilv_node_as_string(name);
		sprintf(title, "L2_%s", nm);
		lilv_node_free(name);
	}
	return 0;
}

int PluginLV2::init_lv2(PluginLV2ClientConfig &conf, int sample_rate)
{
	reset_lv2();

	lv2_AudioPort       = lilv_new_uri(world, LV2_CORE__AudioPort);
	lv2_ControlPort     = lilv_new_uri(world, LV2_CORE__ControlPort);
	lv2_CVPort          = lilv_new_uri(world, LV2_CORE__CVPort);
	lv2_InputPort       = lilv_new_uri(world, LV2_CORE__InputPort);
	lv2_OutputPort      = lilv_new_uri(world, LV2_CORE__OutputPort);
	lv2_Optional        = lilv_new_uri(world, LV2_CORE__connectionOptional);
	atom_AtomPort       = lilv_new_uri(world, LV2_ATOM__AtomPort);
	atom_Sequence       = lilv_new_uri(world, LV2_ATOM__Sequence);
	powerOf2BlockLength = lilv_new_uri(world, LV2_BUF_SIZE__powerOf2BlockLength);
	fixedBlockLength    = lilv_new_uri(world, LV2_BUF_SIZE__fixedBlockLength);
	boundedBlockLength  = lilv_new_uri(world, LV2_BUF_SIZE__boundedBlockLength);
	seq_out = (LV2_Atom_Sequence *) new char[sizeof(LV2_Atom_Sequence) + LV2_SEQ_SIZE];

	conf.init_lv2(lilv, this);
	nb_inputs = nb_outputs = 0;

	for( int i=0; i<conf.nb_ports; ++i ) {
		const LilvPort *lp = lilv_plugin_get_port_by_index(lilv, i);
		if( !lp ) continue;
		int is_input = lilv_port_is_a(lilv, lp, lv2_InputPort);
		if( !is_input && !lilv_port_is_a(lilv, lp, lv2_OutputPort) &&
		    !lilv_port_has_property(lilv, lp, lv2_Optional) ) {
			printf("lv2: not input, not output, and not optional: %s\n", conf.names[i]);
			continue;
		}
		if( is_input && lilv_port_is_a(lilv, lp, lv2_ControlPort) ) {
			conf.append(new PluginLV2Client_Opt(&conf, i));
			continue;
		}
		if( lilv_port_is_a(lilv, lp, lv2_AudioPort) ||
		    lilv_port_is_a(lilv, lp, lv2_CVPort ) ) {
			if( is_input ) ++nb_inputs; else ++nb_outputs;
			continue;
		}
	}

	map.handle = (void*)&uri_table;
	map.map = uri_table_map;
	features.append(new Lv2Feature(LV2_URID_MAP_URI, &map));
	unmap.handle = (void*)&uri_table;
	unmap.unmap  = uri_table_unmap;
	features.append(new Lv2Feature(LV2_URID_UNMAP_URI, &unmap));
	features.append(new Lv2Feature(LV2_BUF_SIZE__powerOf2BlockLength, 0));
	features.append(new Lv2Feature(LV2_BUF_SIZE__fixedBlockLength,    0));
	features.append(new Lv2Feature(LV2_BUF_SIZE__boundedBlockLength,  0));
	features.append(0);

	if( sample_rate < 64 ) sample_rate = 64;
	inst = lilv_plugin_instantiate(lilv, sample_rate, features);
	if( !inst ) {
		printf("lv2: lilv_plugin_instantiate failed\n");
		return 1;
	}

	lilv_instance_activate(inst);
// not sure what to do with these
	max_bufsz = nb_inputs &&
		(lilv_plugin_has_feature(lilv, powerOf2BlockLength) ||
		 lilv_plugin_has_feature(lilv, fixedBlockLength) ||
		 lilv_plugin_has_feature(lilv, boundedBlockLength)) ? 4096 : 0;
	return 0;
}

LV2_URID PluginLV2::uri_table_map(LV2_URID_Map_Handle handle, const char *uri)
{
	return ((PluginLV2UriTable *)handle)->map(uri);
}

const char *PluginLV2::uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
{
	return ((PluginLV2UriTable *)handle)->unmap(urid);
}

void PluginLV2::connect_ports(PluginLV2ClientConfig &conf, int ports)
{
	int ich = 0, och = 0;
	for( int i=0; i<conf.nb_ports; ++i ) {
		const LilvPort *lp = lilv_plugin_get_port_by_index(lilv, i);
		if( !lp ) continue;
		int port = conf.ports[i];
		if( !(port & ports) ) continue;
		if( (port & PORTS_CONTROL) ) {
			lilv_instance_connect_port(inst, i, &conf.ctls[i]);
			continue;
		}
		if( (port & PORTS_AUDIO) ) {
			if( (port & PORTS_INPUT) ) {
				lilv_instance_connect_port(inst, i, in_buffers[ich]);
				iport[ich++] = i;
			}
			else if( (port & PORTS_OUTPUT) ) {
				lilv_instance_connect_port(inst, i, out_buffers[och]);
				oport[och++] = i;
			}
			continue;
		}
		if( (port & PORTS_ATOM) ) {
			if( (port & PORTS_INPUT) )
				lilv_instance_connect_port(inst, i, &seq_in);
			else if( (port & PORTS_OUTPUT) )
				lilv_instance_connect_port(inst, i, seq_out);
			continue;
		}
	}

	seq_in[0].atom.size = sizeof(LV2_Atom_Sequence_Body);
	seq_in[0].atom.type = uri_table.map(LV2_ATOM__Sequence);
	seq_in[1].atom.size = 0;
	seq_in[1].atom.type = 0;
	seq_out->atom.size  = LV2_SEQ_SIZE;
	seq_out->atom.type  = uri_table.map(LV2_ATOM__Chunk);
}

void PluginLV2::del_buffer()
{
	if( shmid >= 0 )
		shm_buffer(-1);

	delete [] in_buffers;  in_buffers = 0;
	delete [] out_buffers;  out_buffers = 0;
}

void PluginLV2::new_buffer(int64_t sz)
{
	uint8_t *bp = 0;
	if( use_shm ) {  // currently, always uses shm
		shmid = shmget(IPC_PRIVATE, sz, IPC_CREAT | 0777);
		if( shmid >= 0 ) {
			bp = (unsigned char*)shmat(shmid, NULL, 0);
			if( bp == (void *) -1 ) { perror("shmat"); bp = 0; }
			shmctl(shmid, IPC_RMID, 0); // delete when last ref gone
		}
		else {
			perror("PluginLV2::allocate_buffer: shmget failed\n");
			BC_Trace::dump_shm_stats(stdout);
		}
	}

	shm_bfr = (shm_bfr_t *) bp;
	if( shm_bfr ) shm_bfr->sz = sz;
}

shm_bfr_t *PluginLV2::shm_buffer(int shmid)
{
	if( this->shmid != shmid ) {
		if( this->shmid >= 0 ) {
			shmdt(shm_bfr);  this->shmid = -1;
			shm_bfr = 0;
		}
		if( shmid >= 0 ) {
			shm_bfr = (shm_bfr_t *)shmat(shmid, NULL, 0);
			if( shm_bfr == (void *)-1 ) { perror("shmat");  shm_bfr = 0; }
			this->shmid = shm_bfr ? shmid : -1;
		}
	}
	return shm_bfr;
}

void PluginLV2::init_buffer(int samples)
{
	int64_t sz = sizeof(shm_bfr_t) +
		sizeof(iport[0])*nb_inputs + sizeof(oport[0])*nb_outputs +
		sizeof(*in_buffers[0]) *samples * nb_inputs +
		sizeof(*out_buffers[0])*samples * nb_outputs;

	if( shm_bfr ) {
		if( shm_bfr->sz < sz ||
		    shm_bfr->nb_inputs != nb_inputs ||
		    shm_bfr->nb_outputs != nb_outputs )
			del_buffer();
	}

	if( !shm_bfr )
		new_buffer(sz);

	shm_bfr->samples = samples;
	shm_bfr->done = 0;
	shm_bfr->nb_inputs = nb_inputs;
	shm_bfr->nb_outputs = nb_outputs;

	map_buffer();
}

// shm_bfr layout:
// struct shm_bfr {
//   int64_t sz;
//   int samples, done;
//   int nb_inputs, nb_outputs;
//   int iport[nb_inputs], 
//   float in_buffers[samples][nb_inputs];
//   int oport[nb_outputs];
//   float out_buffers[samples][nb_outputs];
// };

void PluginLV2::map_buffer()
{
	uint8_t *bp = (uint8_t *)(shm_bfr + 1);

	nb_inputs = shm_bfr->nb_inputs;
	iport = (int *)bp;
	bp += sizeof(iport[0])*nb_inputs;
	in_buffers = new float*[nb_inputs];
	int samples = shm_bfr->samples;
	for(int i=0; i<nb_inputs; ++i ) {
		in_buffers[i] = (float *)bp;
		bp += sizeof(*in_buffers[0])*samples;
	}

	nb_outputs = shm_bfr->nb_outputs;
	oport = (int *)bp;
	bp += sizeof(oport[0])*nb_outputs;
	out_buffers = new float*[nb_outputs];
	for( int i=0; i<nb_outputs; ++i ) {
		out_buffers[i] = (float *)bp;
		bp += sizeof(*out_buffers[0])*samples;
	}
}


#include "file.h"
#include "pluginlv2ui.h"

PluginLV2ChildUI::PluginLV2ChildUI()
 : ForkChild()
{
}

PluginLV2ChildUI::~PluginLV2ChildUI()
{
}

void PluginLV2ChildUI::run()
{
	ArrayList<char *> av;
	av.set_array_delete();
	char arg[BCTEXTLEN];
	const char *exec_path = File::get_cinlib_path();
	snprintf(arg, sizeof(arg), "%s/%s", exec_path, "lv2ui");
	av.append(cstrdup(arg));
	sprintf(arg, "%d", child_fd);
	av.append(cstrdup(arg));
	sprintf(arg, "%d", parent_fd);
	av.append(cstrdup(arg));
	sprintf(arg, "%d", ppid);
	av.append(cstrdup(arg));
	av.append(0);
	execv(av[0], &av.values[0]);
	fprintf(stderr, "execv failed: %s\n %m\n", av.values[0]);
	av.remove_all_objects();
	_exit(1);
}


PluginLV2UI::PluginLV2UI()
 : PluginLV2()
{
	lilv_ui = 0;
	lilv_type = 0;
	uri_map = 0;

	done = -1;
	running = 0;
	updates = 0;
	hidden = 1;
	title[0] = 0;
	child = 0;

// only gtk-2
	gtk_type = "http://lv2plug.in/ns/extensions/ui#GtkUI";
	top_level = 0;
}

PluginLV2UI::~PluginLV2UI ()
{
	reset_gui();
}

#endif
