#!/bin/bash

for i in $*; do
 ( 
  cd $i/constant/polyMesh;
  for j in boundary faces owner neighbour points; do
   O=../../../constant/polyMesh/$j
   if [ -e $O.gz ]; then ln -svf $O.gz .; fi
   if [ -e $O ]; then ln -svf $O .; fi
  done
 )
done
