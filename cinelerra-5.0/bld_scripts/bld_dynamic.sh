#!/bin/bash

dir="$1"
shift
path="/home"
bld="git-repo"
proj="cinelerra5"
base="cinelerra-5.0"

if [ ! -d "$path/$dir/$bld" ]; then
  echo "$bld missing in $path/$dir"
  exit 1
fi

cd "$path/$dir/$bld"
rm -rf "$proj"
git clone "git://git.cinelerra-cv.org/goodguy/cinelerra.git" "$proj"
#rsh host tar -C /mnt0/cinelerra5 -cf - cinelerra | tar -xf -
#mv cinelerra cinelerra5
if [ $? -ne 0 ]; then
  echo "git clone $proj failed"
  exit 1
fi

cd "$proj/$base"
if [ "$dir" = "ubuntu" ]; then
 echo "CFLAGS += -DPNG_SKIP_SETJMP_CHECK=1" >> global_config
fi
if [ "$dir" = "centos" ]; then
 echo "EXTRA_LIBS += -lnuma" >> global_config
 echo "CFLAGS += -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS" >> global_config
fi
if [ "$dir" = "suse" ]; then
 echo "EXTRA_LIBS += -lnuma" >> global_config
fi

STATIC_LIBRARIES=0 ./configure >& log
make >> log 2>&1 $@
make install >> log 2>&1

echo "finished: scanning log for ***"
grep -a "\*\*\*" log

