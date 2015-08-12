#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/oce-0.13.1.tgz && cd oce-0.13.1 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  -DOCE_INSTALL_PREFIX=$INSTALLDIR 
 make -j12
 make install
)
