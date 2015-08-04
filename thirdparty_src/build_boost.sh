#!/bin/bash

if [ ! -x bootstrap.sh ]; then
 echo "please execute from within boost source tree!"
 exit -1
fi

INSTALLDIR=$(cd ../..; pwd)/thirdparty

./bootstrap.sh --prefix=$INSTALLDIR --with-libraries=all
#./b2 install include=$HOME/Programme/bzlib/include linkflags=-L$HOME/Programme/bzlib/lib
./b2 install

