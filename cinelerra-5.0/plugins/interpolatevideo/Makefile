include ../../global_config

OBJS = $(OBJDIR)/interpolatevideo.o \
	$(OBJDIR)/interpolatewindow.o \
	$(OBJDIR)/opticflow.o \
	$(OBJDIR)/motionscan.o

PLUGIN = interpolatevideo

include ../../plugin_config


$(OBJDIR)/interpolatevideo.o: interpolatevideo.C
$(OBJDIR)/interpolatewindow.o: interpolatewindow.C
$(OBJDIR)/opticflow.o: opticflow.C
$(OBJDIR)/motionscan.o: motionscan.C motionscan.h
