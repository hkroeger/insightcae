#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

DIR=boost_1_65_1
#tar xzf TARBALLS/boost_1_65_1.tar.gz && cd boost_1_65_1 && (
[ -d $DIR ] || ( wget -q -O- https://dl.bintray.com/boostorg/release/1.65.1/source/${DIR}.tar.gz | tar xz )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ADDOPT=""
 if [ -e ${INSIGHT_THIRDPARTYDIR}/include/python2.7/Python.h ]; then
  ADDOPT="$ADDOPT --with-python-root=$INSIGHT_THIRDPARTYDIR"
 fi
 ./bootstrap.sh --prefix=$INSTALLDIR --with-libraries=all $ADDOPT
 #./b2 install include=$HOME/Programme/bzlib/include linkflags=-L$HOME/Programme/bzlib/lib
 ./b2 install -j48

)
