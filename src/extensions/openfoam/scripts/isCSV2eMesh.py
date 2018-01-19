#!/usr/bin/env python

import os, sys
import numpy as np

if len(sys.argv)<3:
  print "please provide at least one input file name and the output file name as the last parameter!"
  sys.exit(-1)

ofn=sys.argv[-1]
if not ofn.endswith("eMesh"):
  print "output file name has to end with \"eMesh\"!"
  sys.exit(-1)


allpts=[]
alledgs=[]

i0=0
for fn in sys.argv[1:-1]:
  lines=open(fn, 'r').readlines()[1:]
  pts=[map(float, l.split(',')) for l in lines]
  l=len(pts)
  if len(pts)>=2:
    
    alledgs += [[i,i+1] for i in range(i0, i0+l-1)]
    allpts += pts
    
    i0+=l
  else:
    print "not enough points contained: ignoring", fn
    
open(ofn, 'w').write("""
FoamFile
{
    version     2.0;
    format      ascii;
    class       featureEdgeMesh;
    note        "";
    location    ".";
    object      %s;
}

%d
(
%s
)

%d
(
%s
)
"""%(
  ofn,
  len(allpts),
  '\n'.join(["(%g %g %g)"%(p[0],p[1],p[2]) for p in allpts]),
  len(alledgs),
  '\n'.join(["(%d %d)"%(e[0],e[1]) for e in alledgs])
  ))
