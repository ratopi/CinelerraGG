#!/bin/bash

dir="$1"
path="/home"
bld="google_code"
proj="cinelerra"
base="cinelerra-4.6.mod"

if [ ! -d "$path/$dir/$bld" ]; then
  echo "$bld missing in $path/$dir"
  exit 1
fi

cd "$path/$dir/$bld"
rm -rf "$proj"
git clone "https://code.google.com/p/$proj"
if [ $? -ne 0 ]; then
  echo "git clone $bld/$proj/ failed"
  exit 1
fi

cd "$proj/$base"
if [ "$dir" = "ubuntu" ]; then
 echo "CFLAGS += -DPNG_SKIP_SETJMP_CHECK=1" >> global_config
fi

STATIC_LIBRARIES=1 ./configure >& log
make >> log 2>&1
make install >> log 2>&1

echo "finished: scanning log for ***"
grep -a "\*\*\*" log

