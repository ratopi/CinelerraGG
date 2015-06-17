include ../../global_config

PLUGIN = brighttheme
OBJS = $(OBJDIR)/brighttheme.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/brighttheme.o: brighttheme.C

