#!/bin/bash

OFID=$1
BASHSCR=$2
DATADIR=$3
shift 3 # remove CMD line args, causes problems with some bashrcs

source ${BASHSCR}
if [ -d $OFID ]; then rm -rf $OFID; fi; mkdir $OFID && cd $OFID && (

isofCaseBuilder -sb $DATADIR/simpleFoamCase.iscb &&\
blockMesh && \
isofCaseBuilder -b $DATADIR/simpleFoamCase.iscb $DATADIR/RASmodel_realizableKE.iscb &&\
simpleFoam 

)
