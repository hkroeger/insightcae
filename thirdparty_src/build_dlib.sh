#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

DIR=dlib-18.18
tar xjf TARBALLS/$DIR.tar.bz2 && cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build && cd build && ( cmake ../dlib -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" && make -j12 && make install ) 

 #PYTHONPATH=$INSTALLDIR/lib64/python2.7/site-packages/ python setup.py build -j 12 install --prefix=$INSTALLDIR

)
