#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

#unzip TARBALLS/qwt-6.1.3.zip && cd qwt-6.1.3 && (
DIR=qwt-6.1.3
[ -d $DIR ] || ( wget -q -O- https://sourceforge.net/projects/qwt/files/qwt/6.1.3/qwt-6.1.3.tar.bz2 | tar xj )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 if [ ! -e qwtconfig.pri.bak ]; then
  cp qwtconfig.pri qwtconfig.pri.bak
  sed -i -e "s%QWT_INSTALL_PREFIX *= */usr/local/qwt-\$\$QWT_VERSION%QWT_INSTALL_PREFIX = ${INSTALLDIR}%g" qwtconfig.pri
  sed -i -e "s% *LIBRARY_NAME *= *\$\$1%LIBRARY_NAME = \$\$1-qt5%g" qwtfunctions.pri
  sed -i -e "s/QWT_CONFIG     += QwtDesigner//g" qwtconfig.pri
 fi
 #QMAKEQT5=qmake
 QMAKEQT5=qmake-qt5
 #QMAKEQT4=qmake-qt4
 #if [ ! $(which $QMAKEQT4) ]; then QMAKEQT4=qmake; fi
 $QMAKEQT5 qwt.pro
 make
 make install

)
