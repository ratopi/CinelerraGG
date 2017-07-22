CFLAGS_ := $(CFLAGS)
CXXFLAGS_ := $(CXXFLAGS)
LDFLAGS_ := $(LDFLAGS)

export CFLAGS_ CXXFLAGS_ LDFLAGS_
unexport CFLAGS CXXFLAGS LDFLAGS

AM_MAKEFLAGS := -j$(WANT_JOBS)

AUTOMAKE_OPTIONS=foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = thirdparty libzmpeg3 mpeg2enc mplexlo \
	db guicast cinelerra plugins doc po db/utils

BUILT_SOURCES := bin
bin:
	mkdir -p bin bin/applications bin/pixmaps

clean-generic:
	rm -rf bin

inst_sh := $(CURDIR)/inst.sh

install:	binstall $(sinstall)

uninstall:	buninstall $(suninstall)

.PHONY:	binstall cinstall cuninstall dinstall
export mkinstalldirs install_sh inst_sh

# install to bin
binstall:	install-recursive
	cp -a COPYING README models ffmpeg msg info tips bin/.
	sed -e 's/\<cin\>/$(WANT_CIN)/g' < image/cin.desktop \
		> "bin/applications/$(WANT_CIN).desktop"
	cp -a image/cin.svg "bin/pixmaps/$(WANT_CIN)".svg
	cp -a image/cin.xpm "bin/pixmaps/$(WANT_CIN)".xpm
	cp -a db/utils/new_db "bin/cin_db"

buninstall:
	rm -rf bin

# with system_build
cinstall:
	cd bin; $(inst_sh) "$(DESTDIR)$(bindir)" "$(WANT_CIN)" \
		cin_db zmpeg3{cat,cc2txt,ifochk,show,toc}
	cd bin; $(inst_sh) "$(DESTDIR)$(WANT_CINLIB_DIR)" \
		bdwrite cutads hveg2enc mpeg2enc mplex mplexlo
	cd bin; $(inst_sh) "$(DESTDIR)$(datadir)/$(WANT_CIN)" \
		COPYING README models ffmpeg msg info tips doc
	cd bin/locale; $(inst_sh) "$(DESTDIR)$(localedir)" .
	cd bin/plugins; $(inst_sh) "$(DESTDIR)$(WANT_PLUGIN_DIR)" .
	cd bin/ladspa; $(inst_sh) "$(DESTDIR)$(WANT_LADSPA_DIR)" .
	cd bin/applications; $(inst_sh) "$(DESTDIR)$(datadir)/applications" .
	cd bin/pixmaps; $(inst_sh) "$(DESTDIR)$(datadir)/pixmaps" .

cuninstall:
	rm -f "$(DESTDIR)$(bindir)/$(WANT_CIN)"
	rm -f "$(DESTDIR)$(bindir)"/zmpeg3{cat,cc2txt,ifochk,show,toc}
	rm -f "$(DESTDIR)$(bindir)/cin_db"
	rm -rf "$(DESTDIR)$(WANT_CINLIB_DIR)"
	rm -rf "$(DESTDIR)$(datadir)/$(WANT_CIN)"
	rm -rf "$(DESTDIR)$(localedir)"/*/"LC_MESSAGES/$(WANT_CIN).mo"
	rm -rf "$(DESTDIR)$(WANT_PLUGIN_DIR)"
	rm -rf "$(DESTDIR)$(WANT_LADSPA_DIR)"
	rm -f "$(DESTDIR)$(datadir)/applications/$(WANT_CIN).desktop"
	rm -f "$(DESTDIR)$(datadir)/pixmaps/$(WANT_CIN)".{svg,xpm}

# without system_build
dinstall:
	cp Makefile.devel Makefile

duninstall:
	rm -f Makefile

val-%:
	@echo $($(subst val-,,$@))

