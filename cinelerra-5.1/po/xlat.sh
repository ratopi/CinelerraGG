#!/bin/bash
# this script is used on c++ source to modify the _()
#  gettext calls to use msgqual(ctxt,id) instead.
# local context regions are defined by using:
#
# #undef MSGQUAL
# #define MSGQUAL "qual_id"
# ... code with _() ...
# #undef MSGQUAL
# #define MSGQUAL 0
#
if [ $# -lt 1 -o ! -d "$1" ]; then
  echo 1>&2 "usage: $0 /<path>/cinelerra-src [/tmp] > /<path>/cin.po"
  exit 1
fi

cin_dir=`basename "$1"`
tmp_dir=`mktemp -d -p ${2:-/tmp} cin_XXXXXX`
trap "rm -rf '$tmp_dir'" EXIT

#need a copy of src dir for editing
echo 1>&2 "copy"
cp -a "$1" "$tmp_dir/."
cd "$tmp_dir/$cin_dir"

echo 1>&2 "edit"
for d in guicast/ cinelerra/ plugins/*; do
  if [ ! -d "$d" ]; then continue; fi
  ls -1 $d/*.[Ch] $d/*.inc 2> /dev/null
done | while read f ; do
#qualifier is reset using #define MSGQUAL "qual_id"
#this changes:
#   code C_("xxx") [... code _("yyy")]
#to:
#   code D_("qual_id#xxx") [... code D_("qual_id#yyy")]
  sed -n -i "$f" -f - <<<'1,1{x; s/.*/_("/; x}
:n1 s/^\(#define MSGQUAL[ 	]\)/\1/; t n4
:n2 s/\<C_("/\
/; t n3; P; d; n; b n1
:n3 G; s/^\(.*\)\
\(.*\)\
\(.*\)$/\1\3\2/; b n2
:n4 p; s/^#define MSGQUAL[       ]*"\(.*\)"$/D_("\1#/; t n5
s/^.*$/_("/; h; d; n; b n1
:n5 h; d; n; b n1
'
done

#scan src and generate cin.po
echo 1>&2 "scan"
for d in guicast/ cinelerra/ plugins/*; do
  if [ ! -d "$d" ]; then continue; fi
  ls -1 $d/*.[Ch] $d/*.inc 2> /dev/null
done | xgettext --no-wrap -L C++ -k_ -kN_ -kD_ -f - -o -

echo 1>&2 "done"

