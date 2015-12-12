include ../../global_config

PLUGIN = 5k
OBJS = $(OBJDIR)/5k.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/5k.o: 5k.C

