#!/bin/bash -x

TOPDIR=`pwd`
THIRDPARTY=$TOPDIR/thirdparty
echo "TOPDIR=$TOPDIR" > global_config
echo "THIRDPARTY=$TOPDIR/thirdparty" >> global_config

cpus=`grep -c "^proc" /proc/cpuinfo`
( cd $THIRDPARTY; ./configure cv; \
  export CFG_VARS='CFLAGS+=" -fPIC"'; \
  export MAK_VARS='CFLAGS+=" -fPIC"'; \
  export CFG_PARAMS="--with-pic --enable-pic --disable-asm"; \
  make -j$cpus >& log )

static_libs=`make -C $THIRDPARTY -s val-static_libraries`
static_includes=`make -C $THIRDPARTY -s val-static_includes`

export LDFLAGS=`for f in $static_libs; do
  if [ ! -f "$f" ]; then continue; fi;
  ls $f
done | sed -e 's;/[^/]*$;;' | \
sort -u | while read d; do
 echo -n " -L$d";
done`

export LIBS=`for f in $static_libs; do
  if [ ! -f "$f" ]; then continue; fi;
  ls $f
done | sed -e 's;.*/;;' -e 's;lib\(.*\)\.a$;\1;' | \
sort -u | while read a; do
 echo -n " -l$a";
done`

export CFLAGS="$static_includes"
export CXXFLAGS="$static_includes"

if [ ! -f configure ]; then
  ./autogen.sh
fi
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
  LIBS=`echo "$LIBS" | sed -e "s/[ ]*\<$f\>[ ]*/ /"`
done

echo LDFLAGS=$LDFLAGS
echo LIBS=$LIBS
echo CFLAGS=$CFLAGS

./configure

