#!/bin/bash
name="$1"
lib_path="$2"
headers="$3"
func="$4"
shift 4

: ${TMPDIR:=$TEMPDIR}
: ${TMPDIR:=$TMP}
: ${TMPDIR:=/tmp}

exe=$(mktemp -u "${TMPDIR}/cine-${name}.XXXXXXXX")
trap 'rm -f -- ${exe}' EXIT

{
  for hdr in $headers; do
    test "${hdr%.h}" = "${hdr}" &&
      echo "#include $hdr"    ||
      echo "#include <$hdr>"
  done
  for func in $func; do
    echo "long check_$func(void) { return (long) $func; }"
  done
  echo "int main(void) { return 0; }"
} | cc -x c - -o $exe $@ # >& /dev/null

if [ $? = 0 ]; then
  echo $name=$@
else
  echo $name=$lib_path
fi

