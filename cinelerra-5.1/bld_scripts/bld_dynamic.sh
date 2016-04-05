#!/bin/bash

dir="$1"
shift
path="/home"
bld="git-repo"
proj="cinelerra5"
base="cinelerra-5.1"

if [ ! -d "$path/$dir/$bld" ]; then
  echo "$bld missing in $path/$dir"
  exit 1
fi

cd "$path/$dir/$bld"
rm -rf "$proj"
git clone "git://git.cinelerra-cv.org/goodguy/cinelerra.git" "$proj"
#rsh host tar -C /mnt0 -cf - cinelerra5 | tar -xf -
if [ $? -ne 0 ]; then
  echo "git clone $proj failed"
  exit 1
fi

cd "$proj/$base"
case "$dir" in
  "ubuntu" | "mint" | "ub14" | "ub15")
     echo "CFLAGS += -DPNG_SKIP_SETJMP_CHECK=1" >> global_config ;;
  "centos")
     echo "CFLAGS += -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS" >> global_config
     echo "EXTRA_LIBS += -lnuma" >> global_config ;;
  "suse" | "leap")
     echo "EXTRA_LIBS += -lnuma" >> global_config ;;
  "fedora")
     echo "EXTRA_LIBS += -lnuma" >> global_config ;;
esac

STATIC_LIBRARIES=0 ./configure >& log
make >> log 2>&1 $@
make install >> log 2>&1

echo "finished: scanning log for ***"
grep -ai "\*\*\*.*error" log
