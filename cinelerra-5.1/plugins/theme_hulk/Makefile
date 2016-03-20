include ../../global_config

PLUGIN = theme_hulk
OBJS = $(OBJDIR)/hulktheme.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/hulktheme.o: hulktheme.C

