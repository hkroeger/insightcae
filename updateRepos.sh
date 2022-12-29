#!/bin/bash

set -e
SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
source $SCRIPTPATH/setup_environment.sh

echo ssh reposerver -- -b $BRANCH -l ${BRANCH_NAME} -n ${BUILD_NUMBER} -s ${OS} -v ${VER}
ssh reposerver -- -b $BRANCH -l ${BRANCH_NAME} -n ${BUILD_NUMBER} -s ${OS} -v ${VER}
