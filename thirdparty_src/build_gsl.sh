#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

DIR=gsl-2.4
[ -d $DIR ] || ( wget -q -O- ftp://ftp.gnu.org/gnu/gsl/${DIR}.tar.gz | tar xz )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty
 
 ./configure --prefix=$INSTALLDIR --with-libraries=all
 make -j48
 make install

)
