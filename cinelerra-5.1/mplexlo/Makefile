include ../global_config
include $(THIRDPARTY)/config.mak

OBJS = $(OBJDIR)/mplex.o
CC = gcc
CFLAGS += -I../libzmpeg3
CFLAGS += $(static_includes)
LIBS += ../libzmpeg3/$(OBJDIR)/libzmpeg3.a
LIBS += -Wl,--start-group $(thirdparty_libraries) -Wl,--end-group 
LIBS += -lpthread -lasound -lm -lX11 $(EXTRA_LIBS)

OUTPUT := $(OBJDIR)/mplexlo

$(OUTPUT): $(OBJS)
	@echo $(CXX) -o $(OUTPUT) $(OBJS)
	@$(CXX) -o $(OUTPUT) $(OBJS) $(LIBS)

$(OBJS):
	$(CXX) -c $(CFLAGS) $(subst $(OBJDIR)/,, $*.c) -o $*.o

clean:
	find \( -name core \
		-o -name '*.o' \
		-o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \;
	rm -rf $(OBJDIR)

install:
	cp $(OUTPUT) /usr/bin

$(OBJDIR)/mplex.o: mplex.c
