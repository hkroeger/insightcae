#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/qt-everywhere-opensource-src-4.8.6.tar.gz && cd qt-everywhere-opensource-src-4.8.6 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty
 
 ./configure --prefix=$INSTALLDIR -opensource -confirm-license
 make -j12
 make install
)
