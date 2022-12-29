#!/bin/bash

##
## This script runs inside the build container
##

set -e
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
source $SCRIPTPATH/setup_environment.sh

SRC_PATH=/insight-src
BUILD_PATH=$SRC_PATH/insight-build
ACTION=$1

CMAKE_OPTS+=(
 "-DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_PREFIX"
)



git config --global user.name "Hannes Kroeger"
git config --global user.email "hannes@kroegeronline.net"

#ensure python3 points to custom python, if used
export PATH=/opt/insightcae/bin:$PATH
export INSIGHT_THIRDPARTY_DIR=/opt/insightcae

export MED3HOME=/opt/insightcae/public/med-4.0.0
export HDF5_ROOT=/opt/insightcae/public/hdf5-1.10.3

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$MED3HOME/lib


# system-specific build initializations
case $SYSTEM in
 ubuntu-*)
  CPACK_PACKAGE_GENERATOR=DEB
  if [ -n "$INSTALL_QT5" ]; then
   sudo apt-get clean; sudo rm -rf /var/lib/apt/lists/*; sudo apt-get clean
   sudo apt-get update
   sudo apt install -y qt5-default qttools5-dev qttools5-dev-tools qtxmlpatterns5-dev-tools libqt5svg5-dev libqt5x11extras5-dev libqt5help5 libqt5charts5-dev
  fi
  ;;
 centos-7)
  CPACK_PACKAGE_GENERATOR=RPM
  if [ ! -e /opt/rh ]; then ln -s /opt_rh/rh /opt; fi
  if [ ! -e /usr/include/python3.6 ]; then sudo ln -s /usr/include/python3.6m /usr/include/python3.6; fi # bug in boost 
  source scl_source enable devtoolset-7 || echo ""
  source /usr/share/Modules/init/bash
  module load mpi/openmpi-x86_64
  CMAKE_OPTS+=("-DPTSCOTCH_INCDIR=/usr/include/openmpi-x86_64")
  ;;
 openSUSE-*)
  if [ ! -e /opt/insightcae/lib/libscotch.so ]; then
   sudo mkdir -p /opt/insightcae/lib
   sudo cp -a /usr/local/lib/libscotch*.so /opt/insightcae/lib
   sudo chown $(id -u).$(id -g) /opt/insightcae /opt/insightcae/lib /opt/insightcae/lib/libscotch.*
  fi
  export ARCHFLAGS=-Wno-error=unused-command-line-argument-hard-error-in-future
  export GIT_SSL_NO_VERIFY=true # openfoam's gitlab cert expired
  export PATH=/usr/lib64/mpi/gcc/openmpi/bin:$PATH # mpicc not found thoug mpi-selector called
  CPACK_PACKAGE_GENERATOR=RPM
  ;;
 *)
  echo "UNSUPPORTED SYSTEM: $SYSTEM"
  exit -1
  ;;
esac

cd $BUILD_PATH



if [ "$ACTION" == "build" ]; then

 CMAKE=${CMAKE:-cmake}
 
 #export VERBOSE=99 
 $CMAKE $SRC_PATH -DINSIGHT_BRANCH=${BRANCH} -DINSIGHT_SUPERBUILD=${INSIGHT_THIRDPARTY_DIR} -GNinja ${CMAKE_OPTS[@]}
 
 ninja
 
elif [ "$ACTION" == "TGZ" ]; then
 
 cpack -G TGZ
 
elif [ "$ACTION" == "package" ]; then
 
 cpack -G ${CPACK_PACKAGE_GENERATOR}

else

 cat << EOF
Unknown action: $ACTION!

Valid actions:
 build
 TGZ
 package
 
EOF
 exit -1
fi
