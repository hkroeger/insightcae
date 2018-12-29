#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xJf TARBALLS/armadillo-8.400.1.tar.xz && cd  armadillo-8.400.1 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure -DCMAKE_INSTALL_PREFIX=$INSTALLDIR
 make
 make install
)
