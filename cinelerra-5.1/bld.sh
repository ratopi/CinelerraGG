#!/bin/bash
./autogen.sh
./configure --with-single-user --with-opencv=sta,tar=http://192.168.1.8/opencv-20180401.tgz
make 2>&1 | tee log
make install
mv Makefile Makefile.cfg
cp Makefile.devel Makefile
