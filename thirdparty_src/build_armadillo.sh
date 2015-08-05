#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/armadillo-4.650.4.tar.gz && cd  armadillo-4.650.4 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure -DCMAKE_INSTALL_PREFIX=$INSTALLDIR
 make
 make install
)
