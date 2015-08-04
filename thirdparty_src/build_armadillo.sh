#!/bin/bash

if [ ! -e rcpp_armadillo_csda_2014.pdf ]; then
 echo "please execute from within armadillo source tree!"
 exit -1
fi

INSTALLDIR=$(cd ../..; pwd)/thirdparty

./configure -DCMAKE_INSTALL_PREFIX=$INSTALLDIR
make
make install

