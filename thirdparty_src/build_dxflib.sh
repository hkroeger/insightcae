#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

#tar xzf TARBALLS/dxflib-2.7.5.tar.gz && cd dxflib-2.7.5 && (
DIR=dxflib-2.7.5
[ -d $DIR ] || ( git clone https://${GIT_CREDENTIALS}rostock.silentdynamics.de/git/silentdynamics/caetool-dxflib.git $DIR )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  -DCMAKE_INSTALL_PREFIX=$INSTALLDIR 
 make -j48
 make install
)
