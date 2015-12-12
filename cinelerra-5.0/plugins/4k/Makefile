include ../../global_config

PLUGIN = 4k
OBJS = $(OBJDIR)/4k.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/4k.o: 4k.C

