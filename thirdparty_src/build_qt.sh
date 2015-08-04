#!/bin/bash

if [ ! -e changes-4.8.6 ]; then
 echo "please execute from within QT source tree!"
 exit -1
fi

INSTALLDIR=$(cd ../..; pwd)/thirdparty

./configure --prefix=$INSTALLDIR 
make -j12
make install

