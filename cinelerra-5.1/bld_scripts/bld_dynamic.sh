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
git clone --depth 1 "git://git.cinelerra-cv.org/goodguy/cinelerra.git" "$proj"
#rsh host tar -C /mnt0 -cf - cinelerra5 | tar -xf -
if [ $? -ne 0 ]; then
  echo "git clone $proj failed"
  exit 1
fi

cd "$proj/$base"

./autogen.sh
./configure --enable-static=no
make all install >& log

echo "finished: scanning log for ***"
grep -ai "\*\*\*.*error" log

