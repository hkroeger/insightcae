#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

DIR=numpy-1.11.2

[ -d $DIR ] || ( wget -q -O- https://sourceforge.net/projects/numpy/files/NumPy/1.11.2/${DIR}.tar.gz | tar xz )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 python setup.py build -j 48 install --prefix $INSTALLDIR
)
