include ../../plugin_defs

PLUGIN = theme_unflat
OBJS = $(OBJDIR)/unflattheme.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/unflattheme.o: unflattheme.C

