export TOPDIR ?= $(CURDIR)/..
include $(TOPDIR)/global_config

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
	$(OBJDIR)/vicon.o \
	$(OBJDIR)/workarounds.o

OUTPUT = $(OBJDIR)/libguicast.a

UTILS = $(OBJDIR)/bootstrap $(OBJDIR)/pngtoh $(OBJDIR)/pngtoraw

CFLAGS += -I/usr/include/freetype2

$(shell echo $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo $(OBJS) > $(OBJDIR)/objs)

all: $(OUTPUT) $(UTILS)

$(OBJDIR)/bcxfer.o:	bcxfer.C xfer.C xfer.h
	$(CXX) `cat $(OBJDIR)/c_flags` -O3 -c $< -o $@

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

clean:
	rm -rf $(OBJDIR)
	rm -f bcxfer.C

install:

$(LIBOBJS) $(OBJS) $(CUTOBJ):

$(OBJDIR)/%.o:          %.C
	$(CXX) `cat $(OBJDIR)/c_flags` -c $< -o $@

$(OBJDIR)/bootstrap: bootstrap.c
$(OBJDIR)/pngtoh: pngtoh.c

val-%:
	@echo $($(subst val-,,$@))

