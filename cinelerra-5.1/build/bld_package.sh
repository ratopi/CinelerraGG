#!/bin/bash

if [ $# -ne 2 ]; then
  echo "usage: $0 <os> <typ>"
  echo "  os = centos | fedora | ubuntu | suse"
  echo " typ = static | dynamic"
  exit 1
fi

dir="$1"
path="/home"
bld="google_code"
proj="cinelerra"
base="cinelerra-4.6.mod"

centos="centos-7.0-1406"
fedora="fedora-21"
suse="opensuse-13.2"
ubuntu="ubuntu-14.04.1"

eval os="\${$dir}"
if [ -z "$os" ]; then
  echo "unknown os: $dir"
fi

if [ ! -d "$path/$dir/$bld/$proj/$base" ]; then
  echo "missing $bld/$proj/$base in $path/$dir"
  exit 1
fi

typ=$2
sfx=`uname -m`
if [ "$typ" = "static" ]; then
  sfx="$sfx-static"
elif [ "$typ" != "dynamic" ]; then
  echo "err: suffix must be [static | dynamic]"
  exit 1
fi

cd "$path/$dir/$bld/$proj/$base"
tar -C bin -cJf "../$base-$os-$sfx.txz" .
rm -f "$path/$dir/$base-$os-$sfx.txz"
mv "../$base-$os-$sfx.txz" "$path/$dir/."

