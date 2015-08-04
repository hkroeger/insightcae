#!/bin/bash

if [ ! -d pm3d ]; then
 echo "please execute from within gnuplot source tree!"
 exit -1
fi

INSTALLDIR=$(cd ../..; pwd)/thirdparty

./configure --prefix=$INSTALLDIR
make -j12
make install
