include ../../global_config

PLUGIN = suv
OBJS = $(OBJDIR)/suv.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/suv.o: suv.C

