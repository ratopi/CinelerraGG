
#include "lofwindow.h"

// repeat last output frame on read_frame error
//   uses vframe.status as read status

REGISTER_PLUGIN(LofEffect)

LofConfig::LofConfig()
{
        errs = 0;
        miss = 1;
        mark = 1;
}

void LofConfig::copy_from(LofConfig &src)
{
        errs = src.errs;
        miss = src.miss;
        mark = src.mark;
}

int LofConfig::equivalent(LofConfig &src)
{
        if( errs != src.errs ) return 0;
        if( miss != src.miss ) return 0;
        if( mark != src.mark ) return 0;
	return 1;
}

void LofConfig::interpolate(LofConfig &prev, LofConfig &next, 
	long prev_frame, long next_frame, long current_frame)
{
	copy_from(prev);
}

LofEffect::LofEffect(PluginServer *server)
 : PluginVClient(server)
{
	frm = 0;
}
LofEffect::~LofEffect()
{
	delete frm;
}

const char* LofEffect::plugin_title() { return _("Last Frame"); }
int LofEffect::is_realtime() { return 1; }

NEW_WINDOW_MACRO(LofEffect, LofWindow)
LOAD_CONFIGURATION_MACRO(LofEffect, LofConfig)

void LofEffect::update_gui()
{
	if(thread) {
		thread->window->lock_window();
		load_configuration();
		((LofWindow*)thread->window)->update();
		thread->window->unlock_window();
	}
}


void LofEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->get_data(), MESSAGESIZE);
	output.tag.set_title("LOF");
	output.tag.set_property("ERRS", config.errs);
	output.tag.set_property("MISS", config.miss);
	output.tag.set_property("MARK", config.mark);
	output.append_tag();
	output.terminate_string();
}

void LofEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->get_data(), strlen(keyframe->get_data()));
	while(!input.read_tag()) {
		if(input.tag.title_is("LOF")) {
			config.errs = input.tag.get_property("ERRS", config.errs);
			config.miss = input.tag.get_property("MISS", config.miss);
			config.mark = input.tag.get_property("MARK", config.mark);
		}
	}
}


int LofEffect::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{
	load_configuration();
	int w = frame->get_w(), h = frame->get_h();
	int colormodel = frame->get_color_model();
	if( frm && (frm->get_w() != w || frm->get_h() != h ||
	    frm->get_color_model() != colormodel ) ) {
		delete frm;  frm = 0;
	}
	int ret = read_frame(frame, 0, start_position, frame_rate, get_use_opengl());
	if( ret >= 0 ) ret = frame->get_status();
	if( ret > 0 ) {
		if( !frm )
			frm = new VFrame(w, h, colormodel, -1);
		frm->copy_from(frame);
	}
	else if( !frm )
		frame->clear_frame();
	else if( (ret < 0 && config.errs) || (!ret && config.miss) ) {
		frame->copy_from(frm);
		if( config.mark ) {
			VFrame dot(1, 1, BC_RGBA8888, -1);
			*(uint32_t*)dot.get_rows()[0] = 0xff0000ff;
			int scl = 48, sz = 3, ww = w/scl, hh = h/scl;
			if( ww < sz ) ww = w > sz ? sz : w;
			if( hh < sz ) hh = h > sz ? sz : h;
			BC_CModels::transfer(
				frame->get_rows(), dot.get_rows(),
				frame->get_y(), frame->get_u(), frame->get_v(),
				dot.get_y(), dot.get_u(), dot.get_v(),
				0, 0, 1, 1, 0, 0, ww, hh,
				dot.get_color_model(), frame->get_color_model(),
				0, dot.get_w(), frame->get_w()); 
		}
	}
	return 0;
}


