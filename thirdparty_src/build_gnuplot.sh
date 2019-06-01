#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

DIR=gnuplot-5.2.5
[ -d $DIR ] || ( wget -q -O- https://sourceforge.net/projects/gnuplot/files/gnuplot/5.2.5/${DIR}.tar.gz | tar xz )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ./configure --prefix=$INSTALLDIR --with-texdir=$HOME/texmf
 make -j48
 make install

)
