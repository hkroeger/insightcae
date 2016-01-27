#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/numpy-1.10.4.tar.gz && cd numpy-1.10.4 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 python setup.py build -j 12 install --prefix $INSTALLDIR
)
