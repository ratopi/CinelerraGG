#!/bin/bash

dir="$1"
path="/home"
bld="google_code"
proj="cinelerra"
base="cinelerra-4.6.mod"

if [ ! -d "$path/$dir/$bld/$proj" ]; then
  echo "$bld/$proj missing in $path/$dir"
  exit 1
fi

cd "$path/$dir/$bld/$proj"
git pull
if [ $? -ne 0 ]; then
  echo "git pull $bld/$proj failed"
  exit 1
fi

cd "$base"
make rebuild_all >& log1

echo "finished: scanning log for ***"
grep -a "\*\*\*" log1

