-include ../global_config
include $(THIRDPARTY)/config.mak

OBJDIR := $(shell uname --machine)
USE_MMX = 0
CC = gcc
OBJS := 
INCS :=

ifndef HAVE_CFLAGS
ifeq ($(origin CFLAGS), environment)
HAVE_CFLAGS := y
else
HAVE_CFLAGS := n
endif
endif

MJPEG_DIR = $(shell cd $(THIRDPARTY)/mjpegtools*; pwd)
MPEG3_DIR = $(shell cd ../libzmpeg3*; pwd)

ifeq ($(call firstword,$(encore)),static)
ENCORE_DIR = $(shell cd $(THIRDPARTY)/encore*; pwd)
ENCORE_LIB = $(OBJDIR)/encore
INCS += -DENCORE_INCLUDE=\"$(ENCORE_DIR)/encore.h\"
endif

ifeq ($(call firstword,$(ffmpeg)),static)
FFMPEG_DIR = $(shell cd $(THIRDPARTY)/ffmpeg*; pwd)
FFMPEG_LIB = $(OBJDIR)/ffmpeg/
INCS += -I$(FFMPEG_DIR)
endif

ifeq ($(call firstword,$(faad2)),static)
FAAD_DIR = $(shell cd $(THIRDPARTY)/faad*; pwd)
FAAD_LIB = $(OBJDIR)/faad/
INCS += -I$(FAAD_DIR)/include/
endif

ifeq ($(call firstword,$(faac)),static)
FAAC_DIR = $(shell cd $(THIRDPARTY)/faac*; pwd)
FAAC_LIB = $(OBJDIR)/faac/
INCS += -I$(FAAC_DIR)/include/
endif

ifeq ($(call firstword,$(lame)),static)
LAME_DIR = $(shell cd $(THIRDPARTY)/lame*; pwd)
LAME_LIB = $(OBJDIR)/lame
INCS += -I$(LAME_DIR)/include/
else
INCS += -I/usr/include/lame
endif

ifeq ($(call firstword,$(openjpeg)),static)
OPENJPEG_DIR = $(shell cd $(THIRDPARTY)/openjpeg*; pwd)
OPENJPEG_LIB = $(OBJDIR)/openjpeg
INCS += -I$(OPENJPEG_DIR)/src/lib/openmj2
endif

ifeq ($(call firstword,$(x264)),static)
X264_DIR = $(shell cd $(THIRDPARTY)/x264*; pwd)
X264_LIB = $(OBJDIR)/x264
INCS += -I$(X264_DIR)
endif

ifeq ($(call firstword,$(libdv)),static)
DV_DIR = $(shell cd $(THIRDPARTY)/libdv*; pwd)
DV_LIB = $(OBJDIR)/libdv
INCS += -I$(DV_DIR)/
endif

ifeq ($(call firstword,$(libogg)),static)
OGG_DIR = $(shell cd $(THIRDPARTY)/libogg*; pwd)
INCS += -I$(OBJDIR)/ogg
endif

ifeq ($(call firstword,$(libvorbis)),static)
VORBIS_DIR = $(shell cd $(THIRDPARTY)/libvorbis*; pwd)
VORBIS_LIB = $(OBJDIR)/vorbis
INCS += -I$(VORBIS_DIR)/include -I$(VORBIS_DIR)
endif

ifeq ($(call firstword,$(libjpeg)),static)
JPEG_DIR = $(shell cd $(THIRDPARTY)/libjpeg*; pwd)
JPEG_LIB = $(OBJDIR)/libjpeg
INCS += -I$(JPEG_DIR)
endif

ifneq ($(HAVE_CFLAGS), y)
ifeq ($(OBJDIR), alpha)
  CFLAGS += -O4 -arch ev67 -ieee -accept c99_keywords -gcc_messages
endif

ifeq ($(OBJDIR), i686)
  CFLAGS += -O3 -fmessage-length=0 -funroll-all-loops -fomit-frame-pointer -falign-loops=2 -falign-jumps=2 -falign-functions=2 -I/usr/local/include
endif

ifeq ($(OBJDIR), x86_64)
  CFLAGS += -O2 -funroll-all-loops -fomit-frame-pointer -I/usr/local/include
endif
endif

CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -DHAVE_FIREWIRE

OBJS += \
	$(OBJDIR)/atom.o \
	$(OBJDIR)/avcc.o \
	$(OBJDIR)/avi_hdrl.o \
	$(OBJDIR)/avi_ix.o \
	$(OBJDIR)/avi_idx1.o \
	$(OBJDIR)/avi_indx.o \
	$(OBJDIR)/avi_movi.o \
	$(OBJDIR)/avi_odml.o \
	$(OBJDIR)/avi_riff.o \
	$(OBJDIR)/avi_strl.o \
	$(OBJDIR)/cmodel_default.o \
	$(OBJDIR)/cmodel_float.o \
	$(OBJDIR)/cmodel_yuv420p.o \
	$(OBJDIR)/cmodel_yuv422.o \
	$(OBJDIR)/codecs.o \
	$(OBJDIR)/colormodels.o \
	$(OBJDIR)/ctab.o \
	$(OBJDIR)/dinf.o \
	$(OBJDIR)/dref.o \
	$(OBJDIR)/edts.o \
	$(OBJDIR)/elst.o \
	$(OBJDIR)/esds.o \
	$(OBJDIR)/frma.o \
	$(OBJDIR)/graphics.o \
	$(OBJDIR)/hdlr.o \
	$(OBJDIR)/ima4.o \
	$(OBJDIR)/jpeg.o \
	$(OBJDIR)/libdv.o \
	$(OBJDIR)/libmjpeg.o \
	$(OBJDIR)/matrix.o \
	$(OBJDIR)/mdat.o \
	$(OBJDIR)/mdhd.o \
	$(OBJDIR)/mdia.o \
	$(OBJDIR)/minf.o \
	$(OBJDIR)/moov.o \
	$(OBJDIR)/mp4a.o \
	$(OBJDIR)/mvhd.o \
	$(OBJDIR)/plugin.o \
	$(OBJDIR)/qtasf.o \
	$(OBJDIR)/qtcache.o \
	$(OBJDIR)/qtdv.o \
	$(OBJDIR)/qtpng.o \
	$(OBJDIR)/qtmp3.o \
	$(OBJDIR)/quicktime.o \
	$(OBJDIR)/raw.o \
	$(OBJDIR)/rawaudio.o \
	$(OBJDIR)/rle.o \
	$(OBJDIR)/smhd.o \
	$(OBJDIR)/stbl.o \
	$(OBJDIR)/stco.o \
	$(OBJDIR)/stsc.o \
	$(OBJDIR)/stsd.o \
	$(OBJDIR)/stsdtable.o \
	$(OBJDIR)/stss.o \
	$(OBJDIR)/stsz.o \
	$(OBJDIR)/stts.o \
	$(OBJDIR)/tkhd.o \
	$(OBJDIR)/trak.o \
	$(OBJDIR)/twos.o \
	$(OBJDIR)/udta.o \
	$(OBJDIR)/ulaw.o \
	$(OBJDIR)/util.o \
	$(OBJDIR)/v308.o \
	$(OBJDIR)/v408.o \
	$(OBJDIR)/v410.o \
	$(OBJDIR)/vbraudio.o \
	$(OBJDIR)/vmhd.o \
	$(OBJDIR)/vorbis.o \
	$(OBJDIR)/wmx2.o \
	$(OBJDIR)/workarounds.o \
	$(OBJDIR)/yuv2.o \
	$(OBJDIR)/yuv4.o \
	$(OBJDIR)/yv12.o

FFMPEG_OBJS := \
	$(OBJDIR)/mpeg4.o \
	$(OBJDIR)/qdm2.o \
	$(OBJDIR)/qtffmpeg.o \
	$(OBJDIR)/qth264.o \
	$(OBJDIR)/wma.o


UTILS := \
	$(OBJDIR)/qtdump \
	$(OBJDIR)/make_streamable \
	$(OBJDIR)/qtinfo \
	$(OBJDIR)/dechunk \
	$(OBJDIR)/yuv2mov \
	$(OBJDIR)/yuv4toyuv \
	$(OBJDIR)/recover \
	$(OBJDIR)/rechunk \

#	$(OBJDIR)/mts2mp4


OUTPUT = $(OBJDIR)/libquicktime.a
SHAREDOUTPUT = $(OBJDIR)/libquicktime.so

CFLAGS += -I. \
	-I$(MJPEG_DIR) -I$(MJPEG_DIR)/utils -I$(MJPEG_DIR)/lavtools \
	-I$(MPEG3_DIR) $(INCS) -ffast-math -mno-ieee-fp

FFMPEG_CFLAGS := $(CFLAGS) $(FFMPEG_INCLUDE)

LIBS := $(OUTPUT) $(thirdparty_libraries) $(MPEG3_DIR)/$(OBJDIR)/libzmpeg3.a \
	$(static_libraries) -lpng -lz -lbz2 -llzma -lasound \
	-lX11 -lpthread -ldl -lm -lstdc++


$(shell mkdir -p $(OBJDIR) )
$(shell echo  $(CFLAGS) > $(OBJDIR)/c_flags)
$(shell echo  $(LAME_CFLAGS) > $(OBJDIR)/lame_flags)
$(shell echo > $(OBJDIR)/objs $(OBJS) $(FFMPEG_OBJS) \
	$(foreach lib,$(JPEG_LIB) $(DV_LIB) $(FAAD_LIB) $(FAAC_LIB) $(X264_LIB) \
		 $(OGG_LIB) $(VORBIS_LIB) $(LAME_LIB) $(ENCORE_LIB), $(lib)/*.o))
$(shell echo  $(FFMPEG_CFLAGS) > $(OBJDIR)/ffmpeg_cflags)

all: $(OBJDIR) $(THIRDPARTY) $(OUTPUT)

$(OUTPUT): $(OBJS) $(FFMPEG_OBJS) \
		$(FAAD_LIB) $(FAAC_LIB) $(FFMPEG_LIB) $(X264_LIB) $(DV_LIB) \
		$(JPEG_LIB) $(OGG_LIB) $(VORBIS_LIB) $(LAME_LIB) $(ENCORE_LIB)
	find $(FFMPEG_DIR)/libavcodec/ -name *.o >> $(OBJDIR)/objs
	find $(FFMPEG_DIR)/libavformat/ -name *.o >> $(OBJDIR)/objs
	find $(FFMPEG_DIR)/libavutil/ -name *.o >> $(OBJDIR)/objs
	ar rcs $(OUTPUT) `cat $(OBJDIR)/objs`

# this is the only way we figured out to merge .a files
$(FFMPEG_LIB):
	mkdir -p $(FFMPEG_LIB)/avcodec \
		$(FFMPEG_LIB)/avformat \
		$(FFMPEG_LIB)/avutil
	[ -d $(FFMPEG_DIR) ] && cd $(FFMPEG_LIB)/avcodec && \
	ar x $(FFMPEG_DIR)/libavcodec/libavcodec.a &&  \
	cd ../avformat && \
	ar x $(FFMPEG_DIR)/libavformat/libavformat.a &&  \
	cd ../avutil && \
	ar x $(FFMPEG_DIR)/libavutil/libavutil.a

$(FAAD_LIB):
	mkdir -p $(FAAD_LIB) && \
	[ -d $(FAAD_DIR) ] && cd $(FAAD_LIB) && \
	ar x $(FAAD_DIR)/libfaad/.libs/libfaad.a

$(FAAC_LIB):
	mkdir -p $(FAAC_LIB) && \
	[ -d $(FAAC_DIR) ] && cd $(FAAC_LIB) && \
	ar x $(FAAC_DIR)/libfaac/.libs/libfaac.a

$(DV_LIB):
	mkdir -p $(DV_LIB) && \
	[ -d $(DV_DIR) ] && cd $(DV_LIB) && \
	ar x $(DV_DIR)/libdv/.libs/libdv.a

$(JPEG_LIB):
	mkdir -p $(JPEG_LIB) && \
	[ -d $(JPEB_DIR) ] && cd $(JPEG_LIB) && \
	ar x $(JPEG_DIR)/.libs/libjpeg.a

# need some trickery to get the duplicated filenames out
$(X264_LIB):
	mkdir -p $(X264_LIB) && \
	[ -d $(X264_DIR) ] && cd $(X264_LIB) && \
	ar x $(X264_DIR)/libx264.a && \
	mv predict.o predict2.o && \
	mv cabac.o cabac2.o && \
	mv macroblock.o macroblock2.o && \
	mv set.o set2.o && \
	ar xN 1 $(X264_DIR)/libx264.a predict.o cabac.o macroblock.o set.o

$(OGG_LIB):
	mkdir -p $(OGG_LIB) && \
	cd $(OGG_LIB) && \
	ar x $(OGG_DIR)/src/.libs/libogg.a

$(VORBIS_LIB):
	mkdir -p $(VORBIS_LIB) && \
	[ -d $(VORBIS_DIR) ] && cd $(VORBIS_LIB) && \
	ar x $(VORBIS_DIR)/lib/.libs/libvorbis.a && \
	ar x $(VORBIS_DIR)/lib/.libs/libvorbisenc.a && \
	ar x $(VORBIS_DIR)/lib/.libs/libvorbisfile.a

$(LAME_LIB):
	mkdir -p $(LAME_LIB) && \
	[ -d $(LAME_DIR) ] && cd $(LAME_LIB) && \
	ar x $(LAME_DIR)/libmp3lame/.libs/libmp3lame.a

$(ENCORE_LIB):
	mkdir -p $(ENCORE_LIB) && \
	[ -d $(ENCORE_DIR) ] && cd $(ENCORE_LIB) && \
	ar x $(ENCORE_DIR)/$(OBJDIR)/libencore.a

# broken
$(OPENJPEG_LIB):
	mkdir -p $(OPENJPEG_LIB) && \
	[ -d $(OPENJPEG_DIR) ] && cd $(OPENJPEG_LIB) && \
	ar x $(OPENJPEG_DIR)/bin/libopenjpip.a && \
	ar x $(OPENJPEG_DIR)/bin/libopenjp3d.a && \
	ar x $(OPENJPEG_DIR)/bin/libopenjp2.a

$(OBJDIR)/mts2mp4: $(OBJDIR)/mts2mp4.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/mts2mp4 $(OBJDIR)/mts2mp4.o $(LIBS)

$(OBJDIR)/qtdump: $(OBJDIR)/dump.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/qtdump $(OBJDIR)/dump.o $(LIBS)

$(OBJDIR)/make_streamable: $(OBJDIR)/make_streamable.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/make_streamable $(OBJDIR)/make_streamable.o $(LIBS)

$(OBJDIR)/dechunk: $(OBJDIR)/dechunk.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/dechunk $(OBJDIR)/dechunk.o $(LIBS)

$(OBJDIR)/rechunk: $(OBJDIR)/rechunk.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/rechunk $(OBJDIR)/rechunk.o $(LIBS)

$(OBJDIR)/recover: $(OBJDIR)/recover.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/recover $(OBJDIR)/recover.o $(LIBS)

$(OBJDIR)/yuv4toyuv: $(OBJDIR)/yuv4toyuv.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/yuv4toyuv $(OBJDIR)/yuv4toyuv.o $(LIBS)

$(OBJDIR)/yuv2mov: $(OBJDIR)/yuv2mov.o $(OUTPUT)
	$(CC) -o $(OBJDIR)/yuv2mov $(OBJDIR)/yuv2mov.o $(LIBS)

$(OBJDIR)/qtinfo: $(OBJDIR)/qtinfo.o
	$(CC) -o $(OBJDIR)/qtinfo $(OBJDIR)/qtinfo.o $(LIBS)

utils:  $(UTILS)


clean:
	rm -rf $(OBJDIR)
	find \( -name 'core*' -o -name '*.plugin' \
		-o -name '*.lo' -o -name '*.o' \
		-o -name '*.la' -o -name '*.a' \
		-o -name '*.so' \) -exec rm -f {} \; -prune

backup: clean
	cd .. && \
	tar zcvf quicktime.tar.gz quicktime

install:
	cp $(UTILS) /usr/bin

$(OBJDIR)/%.o:		  %.c
	$(CC) -c `cat $(OBJDIR)/c_flags` $^ -o $@

$(OBJDIR)/mts2mp4.o:
	$(CC) -c `cat $(OBJDIR)/c_flags` $(FFMPEG_INCLUDE) $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(FFMPEG_OBJS): $(OBJDIR)/%.o: %.c
	$(CC) `cat $(OBJDIR)/ffmpeg_cflags` -c $< -o $@

