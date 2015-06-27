include ../global_config

CFLAGS += -I../quicktime

# must only define here so applications using libpng don't see any of the XFT
# includes at all.  They use a conflicting setjmp.
ifeq ($(HAVE_XFT), y)
CFLAGS += -DHAVE_XFT
endif

ifeq ($(OBJDIR), i686)
BOOTSTRAPFLAGS := -DBOOTSTRAP="\"objcopy -B i386 -I binary -O elf32-i386\""
endif

ifeq ($(OBJDIR), x86_64)
BOOTSTRAPFLAGS := -DBOOTSTRAP="\"objcopy -B i386 -I binary -O elf64-x86-64\""
endif


$(shell mkdir -p $(OBJDIR) )

OBJS = \
	$(OBJDIR)/bcbar.o \
	$(OBJDIR)/bcbitmap.o \
	$(OBJDIR)/bcbutton.o \
	$(OBJDIR)/bccapture.o \
	$(OBJDIR)/bccmodels.o \
	$(OBJDIR)/bccounter.o \
	$(OBJDIR)/bcclipboard.o \
	$(OBJDIR)/bcdelete.o \
	$(OBJDIR)/bcdialog.o \
	$(OBJDIR)/bcdisplay.o \
	$(OBJDIR)/bcdisplayinfo.o \
	$(OBJDIR)/bcdragwindow.o \
	$(OBJDIR)/bcfilebox.o \
	$(OBJDIR)/bcfontentry.o \
	$(OBJDIR)/bcwidgetgrid.o \
	$(OBJDIR)/bclistbox.o \
	$(OBJDIR)/bclistboxitem.o \
	$(OBJDIR)/bchash.o \
	$(OBJDIR)/bcmenu.o \
	$(OBJDIR)/bcmenubar.o \
	$(OBJDIR)/bcmenuitem.o \
	$(OBJDIR)/bcmenupopup.o \
	$(OBJDIR)/bcmeter.o \
	$(OBJDIR)/bcnewfolder.o \
	$(OBJDIR)/bcpan.o \
	$(OBJDIR)/bcpbuffer.o \
	$(OBJDIR)/bcpixmap.o \
	$(OBJDIR)/bcpopup.o \
	$(OBJDIR)/bcpopupmenu.o \
	$(OBJDIR)/bcpot.o \
	$(OBJDIR)/bcprogress.o \
	$(OBJDIR)/bcprogressbox.o \
	$(OBJDIR)/bcrecentlist.o \
	$(OBJDIR)/bcrelocatablewidget.o \
	$(OBJDIR)/bcrename.o \
	$(OBJDIR)/bcrepeater.o \
	$(OBJDIR)/bcresources.o \
	$(OBJDIR)/bcscrollbar.o \
	$(OBJDIR)/bcsignals.o \
	$(OBJDIR)/bcslider.o \
	$(OBJDIR)/bcsubwindow.o \
	$(OBJDIR)/bcsynchronous.o \
	$(OBJDIR)/bctextbox.o \
	$(OBJDIR)/bctexture.o \
	$(OBJDIR)/bctheme.o \
	$(OBJDIR)/bctitle.o \
	$(OBJDIR)/bctoggle.o \
	$(OBJDIR)/bctumble.o \
	$(OBJDIR)/bcwindow.o \
	$(OBJDIR)/bcwindow3d.o \
	$(OBJDIR)/bcwindowbase.o \
	$(OBJDIR)/bcwindowdraw.o \
	$(OBJDIR)/bcwindowevents.o \
	$(OBJDIR)/bcxfer.o \
	$(OBJDIR)/condition.o \
	$(OBJDIR)/errorbox.o \
	$(OBJDIR)/filesystem.o \
	$(OBJDIR)/mutex.o \
	$(OBJDIR)/rotateframe.o \
	$(OBJDIR)/sema.o \
	$(OBJDIR)/thread.o \
	$(OBJDIR)/testobject.o \
	$(OBJDIR)/bctimer.o \
	$(OBJDIR)/units.o \
	$(OBJDIR)/vframe.o \
	$(OBJDIR)/vframe3d.o \
	$(OBJDIR)/workarounds.o

OUTPUT = $(OBJDIR)/libguicast.a

TESTLIBS := \
		$(OUTPUT) \
		../quicktime/$(OBJDIR)/libquicktime.a \
		$(LDFLAGS) \
		$(GLLIBS) \
		-lX11 \
		-lXext \
		-lXv \
		-lpthread \
		-lm \
		-lpng

ifeq ($(HAVE_XFT), y)
TESTLIBS += -lXft
endif	

ifeq ($(HAVE_GL), y)
TESTLIBS += -lGL -lGLU
endif

UTILS = $(OBJDIR)/bootstrap $(OBJDIR)/pngtoh $(OBJDIR)/pngtoraw

$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(OBJS) > $(OBJDIR)/objs)

all: $(OUTPUT) $(UTILS)

$(OBJDIR)/bcxfer.o:	bcxfer.C xfer.C xfer.h
	$(CXX) `cat $(OBJDIR)/c_flags` -c $< -o $@

bcxfer.C:	bccmdl.py
	python < ./bccmdl.py > bcxfer.C

$(OUTPUT): $(OBJS)
	ar rcs $(OUTPUT) `cat $(OBJDIR)/objs`

$(OBJDIR)/bootstrap:
	gcc -O2 $(BOOTSTRAPFLAGS) bootstrap.c -o $(OBJDIR)/bootstrap

$(OBJDIR)/pngtoh: pngtoh.c
	gcc -O2 pngtoh.c -o $(OBJDIR)/pngtoh

$(OBJDIR)/pngtoraw: pngtoraw.c
	gcc -O2 pngtoraw.c -o $(OBJDIR)/pngtoraw -lpng -lz

replace:  replace.o $(OUTPUT)
	$(CC) -o replace replace.o $(OUTPUT) \
	../quicktime/i686/libquicktime.a \
	$(LDFLAGS) $(XLIBS) -lpng

#	$(XXF86VM) \



test:	$(OBJDIR)/test.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test \
		$(OBJDIR)/test.o \
		$(TESTLIBS)

test2:	$(OBJDIR)/test2.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test2 \
		$(OBJDIR)/test2.o \
		$(TESTLIBS)

test3:	$(OBJDIR)/test3.o $(STATICOUTPUT)
	$(CC) -o $(OBJDIR)/test3 \
		$(OBJDIR)/test3.o \
		$(TESTLIBS)

clean:
	rm -rf $(OBJDIR)
	rm -f bcxfer.C
	find \( -name core \
		-o -name '*.o' -o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \;

$(LIBOBJS) $(OBJS) $(CUTOBJ):

$(OBJDIR)/%.o:          %.C
	$(CXX) `cat $(OBJDIR)/c_flags` -c $< -o $@

$(OBJDIR)/bootstrap: bootstrap.c
$(OBJDIR)/pngtoh: pngtoh.c
$(OBJDIR)/bccmodel_default.o:	      bccmodel_default.C  bccmodel_permutation.h

val-%:
	@echo $($(subst val-,,$@))

