#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/gsl-1.15.tar.gz && cd gsl-1.15 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty
 
 ./configure --prefix=$INSTALLDIR --with-libraries=all
 make
 make install

)
