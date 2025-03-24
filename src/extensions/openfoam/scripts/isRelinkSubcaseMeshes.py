#!/bin/bash

for i in $*; do
 (
  if [ -d $i ]; then
    cd $i/constant/polyMesh;
    for j in faces owner neighbour points; do
     O=../../../constant/polyMesh/$j
     if [ -e $O.gz ]; then ln -svf $O.gz .; fi
     if [ -e $O ]; then ln -svf $O .; fi
    done
  fi
 )
done
