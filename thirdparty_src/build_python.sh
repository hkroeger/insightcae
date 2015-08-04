#!/bin/bash

if [ ! -e pyconfig.h.in ]; then
 echo "please execute from within python source tree!"
 exit -1
fi

INSTALLDIR=$(cd ../..; pwd)/thirdparty

./configure --prefix=$INSTALLDIR 
make -j12
make install
