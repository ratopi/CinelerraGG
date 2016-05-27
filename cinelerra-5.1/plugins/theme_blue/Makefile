include ../../plugin_defs

PLUGIN = theme_blue
OBJS = $(OBJDIR)/bluetheme.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/bluetheme.o: bluetheme.C

