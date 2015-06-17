include ../../global_config

PLUGIN = defaulttheme
OBJS = $(OBJDIR)/defaulttheme.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
LIBRAW_DIR := $(shell cd ../../thirdparty/libraw1394*; pwd)
CFLAGS += -I../../$(AVC_DIR) -I../../$(LIBRAW_DIR)

include ../../plugin_config

$(OBJDIR)/defaulttheme.o: defaulttheme.C

