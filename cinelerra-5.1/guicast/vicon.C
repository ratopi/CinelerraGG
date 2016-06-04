#include "vicon.h"

#include "bctimer.h"
#include "bcwindow.h"
#include "colors.h"
#include "keys.h"
#include "mutex.h"
#include "condition.h"

VIcon::
VIcon(int vw, int vh, double rate)
{
	this->vw = vw;
	this->vh = vh;
	this->frame_rate = rate;
	this->cycle_start = 0;
	this->age = 0;
	this->seq_no = 0;
	this->in_use = 1;
}

VIcon::
~VIcon()
{
	clear_images();
}

void VIcon::
add_image(VFrame *frm, int ww, int hh, int vcmdl)
{
	VIFrame *vifrm = new VIFrame(ww, hh, vcmdl);
	VFrame *img = *vifrm;
	img->transfer_from(frm);
	images.append(vifrm);
}

void VIcon::
draw_vframe(BC_WindowBase *wdw, int x, int y)
{
	wdw->draw_vframe(frame(), x,y, vw,vh);
}

VIcon *VIconThread::low_vicon()
{
	if( !t_heap.size() ) return 0;
	VIcon *vip = t_heap[0];
	remove_vicon(0);
	return vip;
}

void VIconThread::remove_vicon(int i)
{
	int sz = t_heap.size();
	for( int k; (k=2*(i+1)) < sz; i=k ) {
		if( t_heap[k]->age > t_heap[k-1]->age ) --k;
		t_heap[i] = t_heap[k];
	}
	VIcon *last = t_heap[--sz];
	t_heap.remove_number(sz);
	double age = last->age;
	for( int k; i>0 && age<t_heap[k=(i-1)/2]->age; i=k )
		t_heap[i] = t_heap[k];
	t_heap[i] = last;
}


VIconThread::
VIconThread(BC_WindowBase *wdw, int vw, int vh)
 : Thread(1, 0, 0)
{
	this->wdw = wdw;
	this->view_win = 0;  this->vicon = 0;
	this->view_w = vw;   this->view_h = vh;
	this->viewing = 0;
	draw_lock = new Condition(0, "VIconThread::draw_lock", 1);
	timer = new Timer();
	this->refresh_rate = VICON_RATE;
	done = 0;
	interrupted = -1;
}

VIconThread::
~VIconThread()
{
	stop_drawing();
	done = 1;
	draw_lock->unlock();
	if( Thread::running() ) {
		Thread::cancel();
	}
	Thread::join();
	t_heap.remove_all_objects();
	delete timer;
	delete draw_lock;
}

void VIconThread::
start_drawing()
{
	wdw->lock_window("VIconThread::start_drawing");
	if( view_win )
		wdw->set_active_subwindow(view_win);
        if( interrupted < 0 )
		draw_lock->unlock();
	interrupted = 0;
	wdw->unlock_window();
}

void VIconThread::
stop_drawing()
{
	wdw->lock_window("VIconThread::stop_drawing");
	set_view_popup(0);
	if( !interrupted )
		interrupted = 1;
	wdw->unlock_window();
}

int VIconThread::keypress_event(int key)
{
	if( key != ESC ) return 0;
	set_view_popup(0);
	return 1;
}

int ViewPopup::button_press_event()
{
	vt->set_view_popup(0);
	return 1;
}

bool VIconThread::
visible(VIcon *vicon, int x, int y)
{
	int y0 = 0;
	int my = y + vicon->vh;
	if( my <= y0 ) return false;
	int y1 = y0 + wdw->get_h();
	if( y >= y1 ) return false;
	int x0 = 0;
	int mx = x + vicon->vw;
	if( mx <= x0 ) return false;
	int x1 = x0 + wdw->get_w();
	if( x >= x1 ) return false;
	return true;
}

int ViewPopup::keypress_event()
{
	int key = get_keypress();
	return vt->keypress_event(key);
}
ViewPopup::ViewPopup(VIconThread *vt, VFrame *frame, int x, int y, int w, int h)
 : BC_Popup(vt->wdw, x, y, w, h, BLACK)
{
	this->vt = vt;
}

ViewPopup::~ViewPopup()
{
	vt->wdw->set_active_subwindow(0);
}

void ViewPopup::draw_vframe(VFrame *frame)
{
	BC_WindowBase::draw_vframe(frame, 0,0, get_w(),get_h());
}

ViewPopup *VIconThread::new_view_window(VFrame *frame)
{
	int wx = viewing->get_vx() - view_w, rx = 0;
	int wy = viewing->get_vy() - view_h, ry = 0;
	wdw->get_root_coordinates(wx, wy, &rx, &ry);
	ViewPopup *vwin = new ViewPopup(this, frame, rx, ry, view_w, view_h);
	wdw->set_active_subwindow(vwin);
	return vwin;
}

void VIconThread::
reset_images()
{
	for( int i=t_heap.size(); --i>=0; ) t_heap[i]->reset();
	timer->update();
	img_dirty = win_dirty = 0;
}

void VIconThread::add_vicon(VIcon *vip)
{
	double age = vip->age;
	int i = t_heap.size();  t_heap.append(vip);
	for( int k; i>0 && age<t_heap[(k=(i-1)/2)]->age; i=k )
		t_heap[i] = t_heap[k];
	t_heap[i] = vip;
}

int VIconThread::del_vicon(VIcon *&vicon)
{
	int i = t_heap.size();
	while( --i >= 0 && t_heap[i] != vicon );
	if( i < 0 ) return 0;
	remove_vicon(i);
	delete vicon;  vicon = 0;
	return 1;
}

void VIconThread::set_view_popup(VIcon *vicon)
{
	this->vicon = vicon;
}

int VIconThread::
update_view()
{
	delete view_win;  view_win = 0;
	if( (viewing=vicon) != 0 ) {
		VFrame *frame = viewing->frame();
		view_win = new_view_window(frame);
		view_win->show_window();
	}
	wdw->set_active_subwindow(view_win);
	return 1;
}


void VIconThread::
draw_images()
{
	for( int i=0; i<t_heap.size(); ++i )
		draw(t_heap[i]);
}

void VIconThread::
flash()
{
	if( !img_dirty && !win_dirty ) return;
	if( img_dirty ) wdw->flash();
	if( win_dirty && view_win ) view_win->flash();
	win_dirty = img_dirty = 0;
}

int VIconThread::
draw(VIcon *vicon)
{
	int x = vicon->get_vx(), y = vicon->get_vy();
	int draw_img = visible(vicon, x, y);
	int draw_win = view_win && viewing == vicon ? 1 : 0;
	if( !draw_img && !draw_win ) return 0;
	if( !vicon->frame() ) return 0;
	if( draw_img ) {
		vicon->draw_vframe(wdw, x, y);
		img_dirty = 1;
	}
	if( draw_win ) {
		view_win->draw_vframe(vicon->frame());
		win_dirty = 1;
	}
	return 1;
}

void VIconThread::
run()
{
	while(!done) {
		draw_lock->lock("VIconThread::run 0");
		if( done ) break;
		wdw->lock_window("BC_WindowBase::run 1");
		drawing_started();
		reset_images();
		int64_t seq_no = 0, now = 0;
		int64_t draw_flash = 1000 / refresh_rate;
		while( !interrupted ) {
			if( viewing != vicon )
				update_view();
			VIcon *next = low_vicon();
			while( next && next->age < draw_flash ) {
				now = timer->get_difference();
				if( now >= draw_flash ) break;
				draw(next);
				if( !next->seq_no ) next->cycle_start = now;
				int64_t ref_no = (now - next->cycle_start) / 1000. * refresh_rate;
				int count = ref_no - next->seq_no;
				if( count < 1 ) count = 1;
				ref_no = next->seq_no + count;
				next->age =  next->cycle_start + 1000. * ref_no / refresh_rate;
				if( !next->set_seq_no(ref_no) )
					next->age = now + 1000.;
				add_vicon(next);
				next = low_vicon();
			}
			if( !next ) break;
			add_vicon(next);
			if( draw_flash < now+1 )
				draw_flash = now+1;
			wdw->unlock_window();
			while( !interrupted ) {
				now = timer->get_difference();
				int64_t ms = draw_flash - now;
				if( ms <= 0 ) break;
				if( ms > 100 ) ms = 100;
				Timer::delay(ms);
			}
			wdw->lock_window("BC_WindowBase::run 2");
			now = timer->get_difference();
			int64_t late = now - draw_flash;
			if( late < 1000 ) flash();
			int64_t ref_no = now / 1000. * refresh_rate;
			int64_t count = ref_no - seq_no;
			if( count < 1 ) count = 1;
			seq_no += count;
			draw_flash = seq_no * 1000. / refresh_rate;
		}
		if( viewing != vicon )
			update_view();
		drawing_stopped();
		interrupted = -1;
		wdw->unlock_window();
	}
}

void VIcon::dump(const char *dir)
{
	mkdir(dir,0777);
	for( int i=0; i<images.size(); ++i ) {
		char fn[1024];  sprintf(fn,"%s/img%05d.png",dir,i);
		printf("\r%s",fn);
		VFrame *img = *images[i];
		img->write_png(fn);
	}
	printf("\n");
}

