#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/cmake-2.8.12.2.tar.gz && cd cmake-2.8.12.2 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure --prefix=$INSTALLDIR 
 make 
 make install
)
