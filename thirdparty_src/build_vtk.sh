#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

DIR=VTK-7.1.1

[ -d $DIR ] || ( wget -q -O- https://www.vtk.org/files/release/7.1/${DIR}.tar.gz | tar xz )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  \
 -DCMAKE_C_COMPILER="$(which gcc)"\
 -DCMAKE_CXX_COMPILER="$(which g++)"\
 -DCMAKE_INSTALL_PREFIX="$INSTALLDIR"\
 -DVTK_USE_OFFSCREEN=ON\
 -DVTK_USE_X=OFF\
 -DVTK_OPENGL_HAS_OSMESA=ON\
 -DVTK_QT_VERSION=5 \
 -DBUILD_TESTING=OFF\

 make -j48
 make install
)
