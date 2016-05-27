#!/bin/bash -x

rm -f aclocal.m4 compile install-sh ltmain.sh missing
rm -f config.log config.guess config.h config.h.in config.sub
rm -f global_config configure Makefile Makefile.in depcomp
rm -rf autom4te.cache m4
mkdir m4

autoreconf --install

