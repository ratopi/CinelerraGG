
# to see the make rules:
#   make rules
#
# command uses:
#   ./configure <static/shared>
#   make <name>-clean
#   make <name>
#   make rules
#   make list
#   make val-<var>
#
# to add a new library:
#   vi configure
#     add inc_<name> <inc_paths>
#     add add_library name <lib_paths>
#     add probe call if the library may be already in system
#     fix forced_libs list: for lib in $forced_libs; do
#     add dependencies (order matters): for dep in <pairs>; do
#   vi Makefile
#     add pkg.<data> (pkg.x = .flags, .vars, .params)
#     add $(call rules,$(call std-build,<name>,<dep>...))
#     add deps to other call rules if they require the new library
#     add config enables if other libraries use the new library
#

include ../global_config
include config.mak
BLD := $(THIRDPARTY)/build

pkg-source=$(BLD)/$(1).source
pkg-config=$(BLD)/$(1).configure
pkg-built=$(BLD)/$(1).built

ext=$(lastword $(subst ., ,$(1)))
tarball=$(lastword $(wildcard src/$(1)*.tar.*))
unpack=$(call unpack_$(call ext,$(1)),$(1))
unpack_gz=tar -xzf $(1)
unpack_bz2=tar -xjf $(1)
unpack_xz=tar -xJf $(1)
bld_depends=$(if $(filter $(firstword $($(1))),static),$(call pkg-built,$(1)))
bld_path=`echo $(THIRDPARTY)/$(1)*`/$(2)
inc_path=$(if $(filter $(firstword $($(1))),static),-I$(call bld_path,$(1),$(2)))
ld_path=$(if $(filter $(firstword $($(1))),static),-L$(call bld_path,$(1),$(2)))

#$(eval $(call std-build,pkg,deps...))
#$(pkg.cflags) added as CFLAGS+=$(cflags) to pkg.vars
#$(pkg.cppflags) added as CPPFLAGS+=$(cppflags) to pkg.vars
#$(pkg.cfg_vars) prepended to configure
#$(pkg.cfg_params) appended to configure
#$(pkg.mak_vars) prepended to make
#$(pkg.mak_params) appended to make

define std-build
# $(1)
.PHONY: $(1) $(1)-configure $(1)-src $(1)-clean
$(1): $(call pkg-built,$(1))
$(1)-configure: $(call pkg-config,$(1))
$(1)-src: $(call pkg-source,$(1))
$(1)-clean:
	rm -rf $(1)* $(BLD)/$(1).*

$(call pkg-source,$(1)):
	$(call unpack,$(call tarball,$(1))) \
	$(foreach pch,$(wildcard src/$(1).patch*),
		patch -d $(1)* -p1 < $(pch))
	touch $$@

$(call pkg-config,$(1)): $(call pkg-source,$(1)) $(foreach dep,$(2),$(call bld_depends,$(dep)))
	@echo "CONFIGURING $(1)"
	cd $(1)* && \
		$($(1).cfg_vars) ./configure $($(1).cfg_params)
	touch $$@

$(call pkg-built,$(1)): $(call pkg-config,$(1))
	+$($(1).mak_vars) make -C $(1)* $($(1).mak_params)
	touch $$@

endef

TARGETS := $(static_libs)

all:	$(TARGETS)

$(TARGETS):	$(BLD)

$(BLD):
	mkdir $(BLD)

# vars first
encore.cfg_vars= true ||
esound.cfg_vars:= AUDIOFILE_CFLAGS="$(call inc_path,audiofile,libaudiofile) -laudiofile"
esound.cfg_vars+= AUDIOFILE_LIBS="$(call ld_path,audiofile,libaudiofile/.libs)"
esound.cfg_params= --enable-shared=no --with-pic
esound.ldflags="-lm -lstdc++"
faac.cfg_params= --enable-shared=no
faad2.cfg_params= --with-drm --enable-shared=no
fdk.cfg_vars= ./autogen.sh ;
fdk.cfg_params= --enable-shared=no
fdk.mak_params= ; for f in $(call bld_path,fdk,/lib*/include); do ln -s include `dirname $$$$f`/fdk-aac ; done
fftw.cfg_params= --disable-fortran --enable-shared=no
ffmpeg.cfg_params= \
	--enable-pthreads --enable-gpl \
	--disable-ffserver --disable-ffplay \
	--enable-libfaac \
	--enable-libfdk-aac \
	--enable-libtwolame \
	--enable-libopenjpeg \
	--enable-libmp3lame \
	--enable-libvorbis \
	--enable-libtheora \
	--enable-libvpx \
	--enable-libx264 \
	--enable-libx265 \
	--enable-nonfree \
	--extra-cflags="\
		$(call inc_path,faad2,include) \
		$(call inc_path,faac,include) \
		$(call inc_path,twolame,libtwolame) \
		$(call inc_path,lame,include) \
		$(call inc_path,openjpeg,src/lib/openmj2) \
		$(call inc_path,libvorbis,include) \
		$(call inc_path,libtheora,include) \
		$(call inc_path,libvpx) \
		$(call inc_path,x264) \
		$(call inc_path,x265) \
		$(call inc_path,x265,source) \
		$(call inc_path,fdk,libAACdec) \
		$(call inc_path,fdk,libAACenc) \
		$(call inc_path,fdk,libSYS) \
		$(call inc_path,fdk,libSYS/include)" \
	--extra-cxxflags="-D__STDC_CONSTANT_MACROS" \
	--extra-libs="\
		$(call ld_path,faad2,libfaad/.libs) \
		$(call ld_path,faac,libfaac/.libs) \
		$(call ld_path,twolame,libtwolame/.libs) \
		$(call ld_path,lame,libmp3lame/.libs) \
		$(call ld_path,openjpeg,bin) \
		$(call ld_path,libvorbis,lib/.libs) \
		$(call ld_path,libtheora,lib/.libs) \
		$(call ld_path,libvpx) \
		$(call ld_path,x264) \
		$(call ld_path,x265) \
		$(call ld_path,fdk,.libs) \
		-ldl $(EXTRA_LIBS)" \
	--extra-ldflags="-ldl"

audiofile.cfg_params=--enable-shared=no
audiofile.mak_params=LIBS="-lm -lstdc++"
flac.cfg_params= --enable-shared=no
flac.cflags="$(call inc_path,libogg,include) $(call ld_path,libogg,src/.libs)"
giflib.cfg_params=--enable-shared=no
ilmbase.cfg_params=--prefix=$(call bld_path,ilmbase,usr)
ilmbase.mak_params=; make -C ilmbase* install; cd $(call bld_path,ilmbase); ln -sf lib64 usr/lib
lame.cfg_vars= CFLAGS=-O
lame.cfg_params=--enable-shared=no
lame.mak_params= ; cd $(call bld_path,lame,include); ln -sf . lame
mjpegtools.cflags="$(call inc_path,libjpeg) $(call ld_path,libjpeg,.libs)"
mjpegtools.mak_params=; ln -s . $(call bld_path,mjpegtools,utils)/mjpegtools 
mjpegtools.cfg_params= --enable-shared=no
ladspa.mak_params=; make -C ladspa* install DESTDIR=$(call bld_path,ladspa)
libavc1394.cfg_vars=PKG_CONFIG_PATH=$(bld_path libraw1394)
libavc1394.cflags="$(inc_path libraw1394)"
libavc1394.ldflags="$(ld_path libraw1394,src/.libs)"
libavc1394.cfg_params= --enable-shared=no
libdv.cfg_params= --disable-gtk --enable-shared=no
libiec61883.cfg_vars=PKG_CONFIG_PATH=$(bld_path libraw1394)
libiec61883.cflags="$(inc_path libraw1394)"
libiec61883.ldflags="$(ld_path libraw1394,src/.libs)"
libiec61883.cfg_params= --enable-shared=no
libjpeg.cfg_params= --enable-shared=no
libogg.cfg_params= --enable-shared=no
libraw1394.cfg_params=  --enable-shared=no; ln -sf src libraw1394
libtheora.cfg_vars=PKG_CONFIG_PATH=$(bld_path libogg):$(bld_path libvorbis)
libtheora.cflags="$(inc_path libogg,include) $(inc_path libogg,src) $(inc_path libvorbis,include)"
libtheora.ldflags="$(ld_path libvorbis,lib/.libs) $(ld_path libogg,src/.libs)"
libtheora.cfg_params= --disable-examples --enable-shared=no
libuuid.cfg_params=--enable-shared=no
libvorbis.cfg_params= --disable-oggtest --enable-shared=no
openjpeg.cfg_vars=echo "exec cmake -DBUILD_SHARED_LIBS:BOOL=OFF -DBUILD_MJ2:BOOL=ON ." > ./configure; chmod +x ./configure;
openjpeg.mak_params= openmj2
opencv.cfg_vars=echo "exec cmake -DBUILD_SHARED_LIBS:BOOL=OFF ." > ./configure; chmod +x ./configure;
openexr.cfg_vars=LD_LIBRARY_PATH=$(call bld_path,ilmbase,usr/lib)
openexr.cfg_params=--enable-shared=no
openexr.cfg_params+=--with-ilmbase-prefix=$(call bld_path,ilmbase,usr)
speech_tools.mak_params=-j1
twolame.cfg_params=--enable-shared=no
x264.cfg_params= --enable-static
x265.cfg_vars=echo "exec cmake source -DENABLE_SHARED=no" > ./configure; chmod +x ./configure;

# compile flag vars
$(foreach tgt,$(TARGETS), \
	$(if $($(tgt).cflags),$(eval $(tgt).cfg_vars+=CFLAGS+=$($(tgt).cflags))))
$(foreach tgt,$(TARGETS), \
	$(if $($(tgt).cppflags),$(eval $(tgt).cfg_vars+=CPPFLAGS+=$($(tgt).cppflags))))
$(foreach tgt,$(TARGETS), \
	$(if $($(tgt).ldflags),$(eval $(tgt).cfg_vars+=LDFLAGS+=$($(tgt).ldflags))))

# build rules
.PHONY:	rules
rules:

#$(eval $(call std-build,pkg,deps...))

ifeq ($(MAKECMDGOALS),rules)
rules=$(info $(1))
else
rules=$(eval $(1))
endif

$(call rules,$(call std-build,audiofile))
$(call rules,$(call std-build,encore))
$(call rules,$(call std-build,esound,audiofile))
$(call rules,$(call std-build,faac))
$(call rules,$(call std-build,faad2))
$(call rules,$(call std-build,fdk))
$(call rules,$(call std-build,ffmpeg, faad2 faac fdk twolame lame openjpeg libtheora x264 x265 libvpx))
$(call rules,$(call std-build,fftw))
$(call rules,$(call std-build,flac,libogg))
$(call rules,$(call std-build,giflib))
$(call rules,$(call std-build,ilmbase))
$(call rules,$(call std-build,ladspa))
$(call rules,$(call std-build,lame))
$(call rules,$(call std-build,libavc1394))
$(call rules,$(call std-build,libdv))
$(call rules,$(call std-build,libiec61883))
$(call rules,$(call std-build,libjpeg))
$(call rules,$(call std-build,libogg))
$(call rules,$(call std-build,libraw1394))
$(call rules,$(call std-build,libsndfile))
$(call rules,$(call std-build,libtheora, libogg libvorbis))
$(call rules,$(call std-build,libuuid))
$(call rules,$(call std-build,libvorbis))
$(call rules,$(call std-build,mjpegtools, libjpeg))
$(call rules,$(call std-build,opencv))
$(call rules,$(call std-build,openexr, ilmbase))
$(call rules,$(call std-build,openjpeg))
$(call rules,$(call std-build,speech_tools))
$(call rules,$(call std-build,tiff))
$(call rules,$(call std-build,twolame))
$(call rules,$(call std-build,x264))
$(call rules,$(call std-build,x265))
$(call rules,$(call std-build,libvpx))

# specialize festival, multiple tarballs
festival:
.PHONY: festival festival-configure festival-src
festival: $(call pkg-built,festival)
festival-configure: $(call pkg-config,festival)
festival-src: $(call pkg-source,festival)
festival-clean:
	rm -rf festival* $(BLD)/festival.*

$(call pkg-source,festival):
	for f in src/festival* src/festlex_* src/festvox_*; do tar -xJf $$f; done
	touch $@

$(call pkg-config,festival): $(call pkg-source,festival) $(call pkg-built,speech_tools)
	cd festival* && [ -f ./configure ] && \
		./configure
	touch $@

$(call pkg-built,festival): $(call pkg-config,festival)
	make -C festival* -j1
	touch $@

clean:
	for f in $(thirdparty); do rm -rf $$f*; done
	rm -rf $(BLD)

val-%:
	@echo $($(subst val-,,$@))

