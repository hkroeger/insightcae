#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/boost_1_55_0.tar.gz && cd boost_1_55_0 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 ADDOPT=""
 if [ -e ${INSIGHT_THIRDPARTYDIR}/include/python2.7/Python.h ]; then
  ADDOPT="$ADDOPT --with-python-root=$INSIGHT_THIRDPARTYDIR"
 fi
 ./bootstrap.sh --prefix=$INSTALLDIR --with-libraries=all $ADDOPT
 #./b2 install include=$HOME/Programme/bzlib/include linkflags=-L$HOME/Programme/bzlib/lib
 ./b2 install

)
