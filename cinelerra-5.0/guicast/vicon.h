#ifndef __VICON_H__
#define __VICON_H__

#include "arraylist.h"
#include "bcpopup.h"
#include "bcwindowbase.h"
#include "thread.h"
#include "vicon.inc"
#include "vframe.h"

class ViewPopup : public BC_Popup {
public:
	VIconThread *vt;
	int keypress_event();
	int button_press_event();
	void draw_vframe(VFrame *frame);

	ViewPopup(VIconThread *vt, VFrame *frame, int x, int y, int w, int h);
	~ViewPopup();
};

class VIcon
{
public:
	int vw, vh, vcmdl, in_use;
	ArrayList<VFrame *> images;
        int64_t seq_no;
        double age, period;

	double frame_rate() { return 1000/period; }
	void frame_rate(double r) { period = 1000/r; }
	int64_t vframes() { return images.size(); }
	void clear_images() { images.remove_all_objects(); }

	virtual VFrame *frame() { return images[seq_no]; }
	virtual int64_t next_frame(int n) {
		age += n * period;
		if( (seq_no+=n) >= images.size() ) seq_no = 0;
		return seq_no;
	}
	virtual int get_vx() { return 0; }
	virtual int get_vy() { return 0; }

	void add_image(VFrame *frm, int ww, int hh, int vcmdl);
	void draw_vframe(BC_WindowBase *wdw, int x, int y);
	void dump(const char *dir);

	VIcon(int vw=VICON_WIDTH, int vh=VICON_HEIGHT, double rate=24);
	virtual ~VIcon();
};

class VIconThread : public Thread
{
public:
	int done, interrupted;
	BC_WindowBase *wdw;
	Timer *timer;
	Condition *draw_lock;
	ViewPopup *view_win;
	VIcon *viewing, *vicon;
	int view_w, view_h;
	int img_dirty, win_dirty;
	int64_t draw_flash;

	ArrayList<VIcon *>t_heap;
	VIcon *low_vicon();
	void add_vicon(VIcon *vicon, double age=0);
	int del_vicon(VIcon *&vicon);
	void run();
	void flash();
	int draw(VIcon *vicon);
	int update_view();
	void draw_images();
	void start_drawing();
	void stop_drawing();
	void reset_images();
	void remove_vicon(int i);
	int keypress_event(int key);
	void set_view_popup(VIcon *vicon);

	ViewPopup *new_view_window(VFrame *frame);
	virtual bool visible(VIcon *vicon, int x, int y);
	virtual void drawing_started() {}
	virtual void drawing_stopped() {}

	VIconThread(BC_WindowBase *wdw, int vw=4*VICON_WIDTH, int vh=4*VICON_HEIGHT);
	~VIconThread();
};

#endif
