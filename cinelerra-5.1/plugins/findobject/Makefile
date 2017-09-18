default: all

TOPDIR?=../..
include $(TOPDIR)/plugin_defs

PLUGIN = findobject

OBJS := \
	$(OBJDIR)/findobject.o \
	$(OBJDIR)/findobjectwindow.o \

want_var:=$(WANT_FINDOBJECT)
include $(TOPDIR)/opencv_build
include $(TOPDIR)/plugin_config

all:	opencv
	+make $(OUTPUT)

$(OBJDIR)/findobject.o: findobject.C findobject.h findobjectwindow.h
$(OBJDIR)/findobjectwindow.o: findobjectwindow.C findobject.h findobjectwindow.h

