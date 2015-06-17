include ../../global_config

PLUGIN = chromakeyhsv
SHADER := $(OBJDIR)/chromakey_sl.o
OBJS := $(OBJDIR)/chromakey.o $(SHADER)

include ../../plugin_config

$(OBJDIR)/chromakey.o: chromakey.C
$(SHADER): chromakey.sl
	cd $(OBJDIR) && \
	../../../guicast/$(OBJDIR)/bootstrap -s chromakey_sl.o ../chromakey.sl

