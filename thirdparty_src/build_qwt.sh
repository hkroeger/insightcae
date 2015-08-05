#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xjf TARBALLS/qwt-6.1.2.tar.bz2 && cd qwt-6.1.2 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 if [ ! -e qwtconfig.pri.bak ]; then
  cp qwtconfig.pri qwtconfig.pri.bak
  sed -e "s%QWT_INSTALL_PREFIX *= */usr/local/qwt-\$\$QWT_VERSION%QWT_INSTALL_PREFIX = ${INSTALLDIR}%g" qwtconfig.pri.bak > qwtconfig.pri
 fi

 qmake-qt4 qwt.pro
 make
 make install

)
