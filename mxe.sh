#!/bin/bash

##
## This script runs inside the build container
##

set -e

ACTION=$1

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
source $SCRIPTPATH/configuration.sh

# inside linux cmake-binary dir:
SUPERBUILD_SRC_PATH=/insight-src
SRC_PATH=/insight-src
BUILD_PATH=/insight-windows-build
LINUX_BUILD_PATH=$SCRIPTPATH/insight-build
MXEPATH=/opt/mxe

CMAKE_OPTS+=(
 "-DPYTHON_INCLUDE_DIRS=$MXEPATH/usr/i686-w64-mingw32.shared/python36/include"
 "-DPYTHON_INCLUDE_DIR=$MXEPATH/usr/i686-w64-mingw32.shared/python36/include"
 "-DPYTHON_LIBRARIES=$MXEPATH/usr/i686-w64-mingw32.shared/python36/python36.dll"
 "-DPYTHON_LIBRARY=$MXEPATH/usr/i686-w64-mingw32.shared/python36/python36.dll"
 "-DVTK_ONSCREEN_DIR=$MXEPATH/usr/i686-w64-mingw32.shared/lib/cmake/vtk-8.2"
 "-DCMAKE_WRAPPER=$MXEPATH/usr/bin/i686-w64-mingw32.shared-cmake"
 "-DINSIGHT_BUILD_MEDREADER:BOOL=OFF"
)


export PATH=$PATH:$MXEPATH/usr/bin:$LINUX_BUILD_PATH/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/insightcae/boost-1.65.1/lib

cd $BUILD_PATH

if [ "$ACTION" == "build" ]; then
 
 i686-w64-mingw32.shared-cmake ${CMAKE_OPTS[@]} $SRC_PATH
 make -j

elif [ "$ACTION" == "package" ]; then

 i686-w64-mingw32.static-makensis $SCRIPTPATH/wsl-activation-installer.nsis
 
 $SUPERBUILD_SRC_PATH/generateNSIS.py -c "$REPO_CUSTOMER" -p "$REPO_PASSWORD" -b "$BRANCH" -s "$SRC_PATH"

else

 cat << EOF
Unknown action: $ACTION!

Valid actions:
 build
 package
 
EOF
 exit -1
fi
