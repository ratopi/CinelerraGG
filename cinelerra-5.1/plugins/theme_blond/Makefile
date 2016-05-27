include ../../plugin_defs

PLUGIN = theme_blond
OBJS = $(OBJDIR)/blondtheme.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/blondtheme.o: blondtheme.C

