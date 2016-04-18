
OBJDIR := $(shell uname -m)

TOPDIR=../..
include $(TOPDIR)/global_config
include $(THIRDPARTY)/config.mak

CXXFLAGS:=-ggdb -I.. -Wall $(CFLAGS)
LDLIBS:=../$(OBJDIR)/db.a
LDFLAGS:=

TARGETS:=clip_accesses \
	clip_data \
	clip_del \
	clip_dups \
	clip_ls \
	dbcounts \
	delallclips \
	frame_clips \
	framediff \
	frameweights \
	histeq \
	histplot \
	pbm2frame \
	pbm2key \
	timelinechk \
	clip_assets \
	clip_frames \
	clip_weights \
	dbtv \
	frame2pbm \
	framekey \
	new_db \
	root_info \
	dmp \
	cpdb \
	add_path_pos \
	path_ls \
	dbcvt \
	xtv

all:	$(TARGETS)

dbtv:	dbtv.C
	$(CXX) $(CXXFLAGS) $(LDFLAGS) dbtv.C $(LDLIBS) -lX11 -o $@

XTV_MEDIA_OBJS := $(TOPDIR)/cinelerra/$(OBJDIR)/mediadb.o $(TOPDIR)/cinelerra/$(OBJDIR)/filexml.o
XTV_LIBS := $(TOPDIR)/libzmpeg3/$(OBJDIR)/libzmpeg3.a
XTV_LIBS += $(TOPDIR)/db/$(OBJDIR)/db.a
XTV_LIBS += -Wl,--start-group $(thirdparty_libraries) -Wl,--end-group 
XTV_LIBS += -lX11 -lXext -lasound -lm $(EXTRA_LIBS)
XTV_INCLUDES := -I$(TOPDIR) -I$(TOPDIR)/guicast
XTV_INCLUDES += $(static_includes)

xtv:	xtv.C $(XTV_MEDIA_OBJS)
	@echo c++ -pthread $(CXXFLAGS) xtv.C $(XTV_MEDIA_OBJS) -o xtv
	@c++ -pthread $(XTV_INCLUDES) $(CXXFLAGS) xtv.C $(XTV_MEDIA_OBJS) $(XTV_LIBS) -o xtv

clean:
	rm -f $(TARGETS) *.o a.out

