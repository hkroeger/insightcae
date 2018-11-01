#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xf TARBALLS/qt-everywhere-src-5.11.2.tar.xz && cd qt-everywhere-src-5.11.2 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty
 
 ./configure --prefix=$INSTALLDIR -opensource -confirm-license
 make -j12
 make install
)
