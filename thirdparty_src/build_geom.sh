#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/salomegeom-7.2.0.tar.gz && cd salomegeom-7.2.0 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  -DCMAKE_INSTALL_PREFIX=$INSTALLDIR 
 make -j12
 make install
)
