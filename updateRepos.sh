#!/bin/bash

set -e
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
source $SCRIPTPATH/configuration.sh

echo ssh reposerver -- -b $BRANCH -s ${OS} -v ${VER} -u ${BUILD_URL}
ssh reposerver -- -b $BRANCH -s ${OS} -v ${VER} -u ${BUILD_URL}
