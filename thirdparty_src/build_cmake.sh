#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

#VERSION=2.8.12.2
VERSION=3.5.1

tar xzf TARBALLS/cmake-${VERSION}.tar.gz && cd cmake-$VERSION && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure --prefix=$INSTALLDIR 
 make 
 make install
)
