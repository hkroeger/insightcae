#!/bin/bash

if [ ! -e CMakeLogo.gif ]; then
 echo "please execute from within CMake source tree!"
 exit -1
fi

INSTALLDIR=$(cd ../..; pwd)/thirdparty

./configure --prefix=$INSTALLDIR 
make 
make install
