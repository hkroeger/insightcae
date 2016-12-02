#!/bin/bash

OFID=$1
BASHSCR=$2
DATADIR=$3
shift 3 # remove CMD line args, causes problems with some bashrcs

source ${BASHSCR}
if [ -d $OFID ]; then rm -rf $OFID; fi; mkdir $OFID && cd $OFID && (

isofCaseBuilder -sb $DATADIR/pimpleFoam_pipe.iscb &&\
blockMesh && \
isofCaseBuilder -b $DATADIR/pimpleFoam_pipe.iscb $DATADIR/dynSmagorinsky.iscb &&\
pisoFoam

)
