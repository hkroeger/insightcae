#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

#tar xzf TARBALLS/VTK-6.3.0.tar.gz && 
cd VTK-6.3.0 && (

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
 -DBUILD_TESTING=OFF\

 make -j12
 make install
)
