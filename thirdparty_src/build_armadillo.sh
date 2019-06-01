#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

DIR=armadillo-8.400.1
[ -d $DIR ] || ( wget -q -O- https://sourceforge.net/projects/arma/files/${DIR}.tar.xz | tar xJ ) 
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure -DCMAKE_INSTALL_PREFIX=$INSTALLDIR
 make -j48
 make install
)
