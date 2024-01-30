#!/bin/bash

##
## This script runs inside the build container
##

set -e

ACTION=$1

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
source $SCRIPTPATH/configuration.sh

CMAKE_OPTS+=(
 "-DCMAKE_BUILD_TYPE=Release"
 "-DINSIGHT_BUILD_PYTHONBINDINGS=OFF"
 "-DDXFLIB_INCLUDE_DIR:PATH=/mxe/usr/i686-w64-mingw32.shared/include/dxflib"
 "-DDXFLIB_LIBRARY:PATH=/mxe/usr/i686-w64-mingw32.shared/lib/libdxflib.dll.a"
)

# inside linux cmake-binary dir:
SUPERBUILD_SRC_PATH=/insight-src
SRC_PATH=/insight-src
BUILD_PATH=/insight-windows-build
#LINUX_BUILD_PATH=/opt/insight-build/insight/build

export PATH=$PATH:/mxe/usr/bin:$BUILD_PATH/bin

source /opt/insightcae/bin/insight_setenv.sh


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
