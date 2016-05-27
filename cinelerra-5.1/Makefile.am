TOPDIR ?= $(CURDIR)
-include global_config

AM_MAKEFLAGS := -j$(WANT_JOBS)

AUTOMAKE_OPTIONS=foreign
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = thirdparty libzmpeg3 mpeg2enc mplexlo \
        db guicast cinelerra plugins doc po

BUILT_SOURCES := bin
bin:
	mkdir -p $@

clean-generic:
	rm -rf bin

install:	install-recursive
	cp COPYING README $(BINDIR)/.
	cp -a models $(BINDIR)/.
	cp -a ffmpeg $(BINDIR)/.
	cp -a msg.txt $(BINDIR)/.

rebuild:
	$(MAKE) -C guicast clean
	$(MAKE) -C cinelerra clean
	$(MAKE) -C plugins clean
	$(MAKE) -C guicast
	$(MAKE) -C cinelerra
	$(MAKE) -C plugins

rebuild_all:
	$(MAKE) -C libzmpeg3 clean
	$(MAKE) -C libzmpeg3
	$(MAKE) -C db clean
	$(MAKE) -C db
	$(MAKE) rebuild
	$(MAKE) -C db/utils

rebuild_install:
	$(MAKE) -C cinelerra install
	$(MAKE) -C plugins install

val-%:
	 @echo $($(subst val-,,$@))

