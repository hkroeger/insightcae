#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xf TARBALLS/qt-everywhere-opensource-src-5.9.7.tar.xz && cd qt-everywhere-opensource-src-5.9.7 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty
 
 ./configure --prefix=$INSTALLDIR -opensource -confirm-license
 make -j16
 make install
)
