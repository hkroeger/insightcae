#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/gnuplot-5.2.5.tar.gz && cd gnuplot-5.2.5 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure --prefix=$INSTALLDIR --with-texdir=$HOME/texmf
 make -j12
 make install

)
