#!/bin/bash

if [ ! -e qwt.pro ]; then
 echo "please execute from within Qwt source tree!"
 exit -1
fi

INSTALLDIR=$(cd ../..; pwd)/thirdparty

if [ ! -e qwtconfig.pri.bak ]; then
 cp qwtconfig.pri qwtconfig.pri.bak
 sed -e "s%QWT_INSTALL_PREFIX *= */usr/local/qwt-\$\$QWT_VERSION%QWT_INSTALL_PREFIX = ${INSTALLDIR}%g" qwtconfig.pri.bak > qwtconfig.pri
fi
qmake-qt4 qwt.pro
make
make install

