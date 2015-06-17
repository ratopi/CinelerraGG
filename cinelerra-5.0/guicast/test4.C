
//c++ -g -I../guicast testwindow.C ../guicast/x86_64/libguicast.a \
// -DHAVE_GL -DHAVE_XFT -I/usr/include/freetype2 -lGL -lX11 -lXext \
// -lXinerama -lXv -lpng  -lfontconfig -lfreetype -lXft -pthread

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bcwindow.h"
#include "bcsignals.h"
#include "colors.h"
#include "fonts.h"
#include "thread.h"
#include "vframe.h"

class TestWindowGUI : public BC_Window
{
public:

	TestWindowGUI(int x, int y, int w, int h);
	~TestWindowGUI();
};


class TestWindow : public Thread
{
	TestWindowGUI *gui;
public:
	TestWindow(int x, int y, int w, int h)
	 : Thread(1,0,0) {
		gui = new TestWindowGUI(x,y,w,h);
		start();
	}
	void draw(VFrame *vframe) {
		gui->draw_vframe(vframe,
			0,0,gui->get_w(),gui->get_h(),
			0,0,vframe->get_w(),vframe->get_h(),
			0);
		gui->flash();
	}
	void show_text(int tx, int ty, const char *fmt, ...);
	void close_window() { gui->close(0); }
	~TestWindow() { delete gui; }
	void run() { gui->run_window(); }
};

TestWindowGUI::
TestWindowGUI(int x, int y, int w, int h)
 : BC_Window("test", x,y, w,h, 100,100)
{
	set_bg_color(BLACK);
	clear_box(0,0,get_w(),get_h());
	flash();
	set_font(MEDIUMFONT);
}

TestWindowGUI::
~TestWindowGUI()
{
}

void TestWindow::show_text(int tx, int ty, const char *fmt, ...)
{
	char text[1024];
        va_list ap;
        va_start(ap, fmt);
        vsprintf(text, fmt, ap);
        va_end(ap);
	gui->set_color(0xc080f0);
	gui->draw_text(tx,ty, text);
	gui->flash();
}

const char *cmdl[] = {
 "transparency", "compressed", "rgb8", "rgb565", "bgr565", "bgr888", "bgr8888", "yuv420p",
 "none", "rgb888", "rgba8888", "rgb161616", "rgba16161616", "yuv888", "yuva8888", "yuv161616",
 "yuva16161616", "yuv422p", "yuv411p", "yuv422", "argb8888", "abgr8888", "a8", "a16",
 "yuv101010", "vyu888", "uyva8888", "yuv444p", "yuv410p", "rgb_float", "rgba_float", "a_float",
 "rgb_floatp",
};

void write_pbm(uint8_t *tp, int w, int h, const char *fmt, ...)
{
  va_list ap;    va_start(ap, fmt);
  char fn[256];  vsnprintf(fn, sizeof(fn), fmt, ap);
  va_end(ap);
  FILE *fp = !strcmp(fn,"-") ? stdout : fopen(fn,"w");
  if( fp ) {
    fprintf(fp,"P5\n%d %d\n255\n",w,h);
    fwrite(tp,w,h,fp);
    fclose(fp);
  }
}

void write_pgm(uint8_t *tp, int w, int h, const char *fmt, ...)
{
  va_list ap;    va_start(ap, fmt);
  char fn[256];  vsnprintf(fn, sizeof(fn), fmt, ap);
  va_end(ap);
  FILE *fp = !strcmp(fn,"-") ? stdout : fopen(fn,"w");
  if( fp ) {
    fprintf(fp,"P6\n%d %d\n255\n",w,h);
    fwrite(tp,3*w,h,fp);
    fclose(fp);
  }
}

int main(int ac, char **av)
{
	BC_Signals signals;
	signals.initialize();
	int fd = open("test.png",O_RDONLY);
	if( fd < 0 ) exit(1);
	struct stat st;  fstat(fd,&st);
	unsigned char *dat = new unsigned char[st.st_size];
	read(fd, dat, st.st_size);
	VFrame ifrm(dat, st.st_size);
	delete [] dat;
	close(fd);
	int w = ifrm.get_w(), h = ifrm.get_h();
	TestWindow test_window(100, 100, w, h);
	for( int fr_cmdl=1; fr_cmdl<=32; ++fr_cmdl ) {
		if( fr_cmdl == BC_TRANSPARENCY || fr_cmdl == BC_COMPRESSED ) continue;
		if( fr_cmdl == BC_A8 || fr_cmdl == BC_A16 ) continue;
		if( fr_cmdl == BC_A_FLOAT || fr_cmdl == 8 ) continue;
		VFrame afrm(w, h, fr_cmdl, -1);
		afrm.transfer_from(&ifrm, 0);
		for( int to_cmdl=1; to_cmdl<=32; ++to_cmdl ) {
			if( to_cmdl == BC_TRANSPARENCY || to_cmdl == BC_COMPRESSED ) continue;
			if( to_cmdl == BC_A8 || to_cmdl == BC_A16 ) continue;
			if( to_cmdl == BC_A_FLOAT || to_cmdl == 8 ) continue;
			VFrame bfrm(w, h, to_cmdl, -1);
			bfrm.transfer_from(&afrm, 0);
			test_window.draw(&bfrm);
			VFrame cfrm(w, h, BC_RGB888, -1);
			cfrm.transfer_from(&bfrm, 0);
			printf("xfer_%s_to_%s\n",cmdl[fr_cmdl],cmdl[to_cmdl]);
			test_window.show_text(50,50, "xfer_%s_to_%s",cmdl[fr_cmdl],cmdl[to_cmdl]);
			write_pgm(cfrm.get_data(), w,h, "/tmp/test/xfer_%s_to_%s.pgm",
				cmdl[fr_cmdl],cmdl[to_cmdl]);
			//usleep(100000);
		}
	}
	test_window.close_window();
	test_window.join();
	return 0;
}

