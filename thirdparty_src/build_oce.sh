#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

#tar xzf TARBALLS/oce-0.13.1.tgz && cd oce-0.13.1 && (
DIR=opencascade-7.2
[ -d $DIR ] || ( git clone git clone ssh://gogs@rostock.silentdynamics.de:222/silentdynamics/caetool-opencascade.git $DIR )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  -DOCE_INSTALL_PREFIX=$INSTALLDIR 
 make -j48
 make install
)
