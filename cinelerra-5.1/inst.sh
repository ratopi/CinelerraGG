#!/bin/bash -e
# inst.sh <dir> <objs...>

dir="$1"; shift 1
$mkinstalldirs "$dir"

for f in "$@"; do
  if [ -f "$f" ]; then $install_sh -c "$f" "$dir"; continue; fi
  if [ -d "$f" ]; then ( cd $f; $inst_sh "$dir/$f" * )
  else echo "*** Error - install $f in $dir failed." 1>&2; exit 1; fi
done
