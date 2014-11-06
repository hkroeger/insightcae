#!/bin/bash

for i in $*; do
 ( 
  cd $i/constant/polyMesh;
  for j in boundary faces owner neighbour points; do
   if [ -h $j ]; then ln -svf ../../../constant/polyMesh/$j .; fi
   if [ -h $j.gz ]; then ln -svf ../../../constant/polyMesh/$j.gz .; fi
  done
 )
done
