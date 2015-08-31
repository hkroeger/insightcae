#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

tar xzf TARBALLS/ParaView-v4.3.1-source.tar.gz && cd ParaView-v4.3.1-source && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  \
 -DCMAKE_INSTALL_PREFIX=$INSTALLDIR\
 -DPARAVIEW_ENABLE_PYTHON=ON\
 -DVTK_USE_X=OFF\
 -DVTK_OPENGL_HAS_OSMESA=ON\
 -DPARAVIEW_BUILD_QT_GUI=OFF\

 make -j12
 make install
)
