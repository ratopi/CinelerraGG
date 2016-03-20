include ../../global_config

PLUGIN = theme_pinklady
OBJS = $(OBJDIR)/pinkladytheme.o
OUTPUT_THEME = $(OUTPUT)

AVC_DIR := $(shell cd ../../thirdparty/libavc*; pwd)
CFLAGS += -I$(AVC_DIR)

include ../../plugin_config

$(OBJDIR)/pinkladytheme.o: pinkladytheme.C

