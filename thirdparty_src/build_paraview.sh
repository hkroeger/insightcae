#!/bin/bash

if [ ! -d TARBALLS ]; then
 echo "please execute from within thirdparty_src directory!"
 exit -1
fi

DIR=paraview-4.4.0
[ -d $DIR ] || ( git clone ssh://gogs@rostock.silentdynamics.de:222/silentdynamics/caetool-paraview.git $DIR )
cd $DIR && (

 INSTALLDIR=$(cd ../..; pwd)/thirdparty

 mkdir build
 cd build
 cmake ..  \
 -DCMAKE_C_COMPILER="$(which gcc)"\
 -DCMAKE_CXX_COMPILER="$(which g++)"\
 -DCMAKE_INSTALL_PREFIX="$INSTALLDIR"\
 -DPARAVIEW_ENABLE_PYTHON=ON\
 -DVTK_USE_X=OFF\
 -DVTK_OPENGL_HAS_OSMESA=ON\
 -DPARAVIEW_BUILD_QT_GUI=OFF\
 -DBUILD_TESTING=OFF\
 -DPARAVIEW_INSTALL_DEVELOPMENT_FILES=ON\
 -DPARAVIEW_DATA_EXCLUDE_FROM_ALL=ON

 make -j48
 make install
)
