#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

#VERSION=2.8.12.2
VERSION=3.10.2
DIR=cmake-$VERSION

#tar xzf TARBALLS/cmake-${VERSION}.tar.gz && cd cmake-$VERSION && (
[ -d $DIR ] || ( wget -q -O- https://cmake.org/files/v3.10/${DIR}.tar.gz | tar xz )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure --prefix=$INSTALLDIR 
 make -j48
 make install
)
