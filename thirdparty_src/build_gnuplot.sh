#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/gnuplot-4.6.7.tar.gz && cd gnuplot-4.6.7 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure --prefix=$INSTALLDIR
 make -j12
 make install

)
