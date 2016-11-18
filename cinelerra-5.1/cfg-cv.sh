#!/bin/bash -x
# cd cincv;  cfg_cv.sh /path/cin5
cin="$1"
THIRDPARTY=`pwd`/thirdparty

rm -rf thirdparty; cp -a $cin/thirdparty .
for f in configure.ac Makefile.am autogen.sh; do mv $f $f.cv; cp -a $cin/$f .; done
mv m4 m4.cv
rm -rf ./libzmpeg3 ./db
mkdir libzmpeg3 db db/utils

./autogen.sh
./configure --disable-static-build --without-ladspa-build \
  --enable-faac=yes --enable-faad2=yes --enable-a52dec=yes \
  --enable-mjpegtools=yes --enable-lame=yes --enable-x264=yes \
  --enable-libogg=auto --enable-libtheora=auto --enable-libvorbis=auto \
  --enable-openexr=auto --enable-libsndfile=auto --enable-libdv=auto \
  --enable-libjpeg=auto --enable-tiff=auto --enable-x264=auto \
  --disable-audiofile --disable-encore --disable-esound --disable-fdk \
  --disable-ffmpeg --disable-fftw --disable-flac --disable-giflib --disable-ilmbase \
  --disable-libavc1394 --disable-libraw1394 --disable-libiec61883 --disable-libvpx \
  --disable-openjpeg --disable-twolame --disable-x265

export CFG_VARS='CFLAGS+=" -fPIC"'; \
export MAK_VARS='CFLAGS+=" -fPIC"'; \
export CFG_PARAMS="--with-pic --enable-pic --disable-asm"; \

jobs=`make -s -C thirdparty val-WANT_JOBS`
make -C thirdparty -j$jobs

static_libs=`make -C thirdparty -s val-static_libs`
static_incs=`make -C thirdparty -s val-static_incs`

./autogen.sh clean
for f in configure.ac Makefile.am autogen.sh; do rm -f $f; mv $f.cv $f; done
mv m4.cv m4

export LDFLAGS=`for f in $static_libs; do
  if [ ! -f "$f" ]; then continue; fi;
  ls $f
done | sed -e 's;/[^/]*$;;' | \
sort -u | while read d; do
 echo -n " -L$d";
done`

export LIBS=-lpthread `for f in $static_libs; do
  if [ ! -f "$f" ]; then continue; fi;
  ls $f
done | sed -e 's;.*/;;' -e 's;lib\(.*\)\.a$;\1;' | \
sort -u | while read a; do
 echo -n " -l$a";
done`

export CFLAGS="$static_incs"
export CXXFLAGS="$static_incs"

if [ ! -f configure ]; then ./autogen.sh; fi
sed -e 's/^LIBX264_LIBS=""/#LIBX264_LIBS=""/' -i configure

export MJPEG_LIBS="-L$THIRDPARTY/mjpegtools-2.1.0/utils/.libs -lmjpegutils \
  -L$THIRDPARTY/mjpegtools-2.1.0/lavtools/.libs -llavfile \
  -L$THIRDPARTY/mjpegtools-2.1.0/lavtools/.libs -llavjpeg \
  -L$THIRDPARTY/mjpegtools-2.1.0/mpeg2enc/.libs -lmpeg2encpp \
  -L$THIRDPARTY/mjpegtools-2.1.0/mplex/.libs -lmplex2"
export MJPEG_CFLAGS="-I$THIRDPARTY/mjpegtools-2.1.0/. \
  -I$THIRDPARTY/mjpegtools-2.1.0/lavtools \
  -I$THIRDPARTY/mjpegtools-2.1.0/utils"

export LIBX264_CFLAGS="-I$THIRDPARTY/x264-20151229/."
export LIBX264_LIBS="-L$THIRDPARTY/x264-20151229/. -lx264"

for f in $MJPEG_LIBS $LIBX264_LIBS; do
  LIBS=`echo "$LIBS" | sed -e "s;[ ]*\<$f\>[ ]*; ;"`
done

echo LDFLAGS=$LDFLAGS
echo LIBS=$LIBS
echo CFLAGS=$CFLAGS

./configure

#make -j$jobs >& log
#make install DESTDIR=`pwd` >> log 2>&1
#export LD_LIBRARY_PATH=`pwd`/usr/local/lib
#cd cinelerra
#gdb ./.libs/cinelerra

