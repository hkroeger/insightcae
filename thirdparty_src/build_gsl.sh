#!/bin/bash

if [ ! -e gsl-config.in ]; then
 echo "please execute from within GSL source tree!"
 exit -1
fi

INSTALLDIR=$(cd ../..; pwd)/thirdparty

./configure --prefix=$INSTALLDIR --with-libraries=all
make
make install

