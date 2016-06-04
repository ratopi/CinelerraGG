#!/bin/bash
./autogen.sh
./configure --with-single-user
make 2>&1 | tee log
make install
mv Makefile Makefile.cfg
cp Makefile.devel Makefile
