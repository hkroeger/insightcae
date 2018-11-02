#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

unzip TARBALLS/qwt-6.1.3.zip && cd qwt-6.1.3 && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 if [ ! -e qwtconfig.pri.bak ]; then
  cp qwtconfig.pri qwtconfig.pri.bak
  sed -e "s%QWT_INSTALL_PREFIX *= */usr/local/qwt-\$\$QWT_VERSION%QWT_INSTALL_PREFIX = ${INSTALLDIR}%g" qwtconfig.pri.bak > qwtconfig.pri
 fi
 if [ ! -e qwtfunctions.pri.bak ]; then
  cp qwtfunctions.pri qwtfunctions.pri.bak
  sed -e "s% *LIBRARY_NAME *= *\$\$1%LIBRARY_NAME = \$\$1-qt5%g" qwtfunctions.pri.bak > qwtfunctions.pri
 fi
 QMAKEQT5=qmake
 #QMAKEQT4=qmake-qt4
 #if [ ! $(which $QMAKEQT4) ]; then QMAKEQT4=qmake; fi
 $QMAKEQT5 qwt.pro
 make
 make install

)
