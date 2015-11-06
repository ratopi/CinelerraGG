#ifndef LOF_H
#define LOF_H

#include "filexml.h"
#include "bchash.h"
#include "language.h"
#include "pluginvclient.h"
#include "vframe.h"

class LofConfig;
class LofEffect;
class LofWindow;

class LofConfig {
public:
        int errs;
        int miss;
        int mark;

	LofConfig();

	void copy_from(LofConfig &src);
	int equivalent(LofConfig &src);
	void interpolate(LofConfig &prev, LofConfig &next, 
		long prev_frame, long next_frame, long current_frame);
};

class LofEffect : public PluginVClient {
public:
	LofEffect(PluginServer *server);
	~LofEffect();

	PLUGIN_CLASS_MEMBERS(LofConfig)
	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();

	VFrame *frm;
};


#endif
