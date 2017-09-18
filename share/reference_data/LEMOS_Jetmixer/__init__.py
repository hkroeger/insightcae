#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory


def getToC():
  xDJ=[0, 0.1, 0.6, 1.1, 2.1, 3.1, 5.1, 7.1, 9.1]
  xDJ_Ffilt=[0, 0.5, 1.0, 2.0, 3.0, 5.0, 7.0, 9.0]
  xDR=[0, 0.1, 0.6, 1.1, 1.6, 2.1, 2.6, 3.1, 5.1, 7.1, 9.1]
  toc={
      
    "J-Mode/U_vs_xD": None,
    "J-Mode/uPrime_vs_xD": None,
    "J-Mode/U_vs_rD@": xDJ,
    "J-Mode/uPrime_vs_rD@": xDJ,
    "J-Mode/G_vs_rD@": xDJ_Ffilt,
    "J-Mode/gPrime_vs_rD@": xDJ_Ffilt,
    
    "R-Mode/U_vs_xD": None,
    "R-Mode/uPrime_vs_xD": None,
    "R-Mode/U_vs_rD@": xDR,
    "R-Mode/uPrime_vs_rD@": xDR
    
    }
  return toc
  
def getProfile(name):
  cp=name.split('/')
  
  mode=cp[0]
  
  m=re.search("^(.+)_vs_(.+)$", cp[1])
  yname=m.group(1)
  xname=m.group(2)
  
  fname=None
  xcol=None
  ycol=None
  op=None
  
	
  xcol=0
  if yname=="U":
    ycol=1
  elif yname=="uPrime":
    ycol=2
  elif yname=="G":
    ycol=1
  elif yname=="gPrime":
    ycol=2
    
  msn="jm" if mode=="J-Mode" else "rm"
  
  fname=None
  if xname.startswith("rD@"):
    xD=float(xname.split('@')[1])      
    if (yname=="U") or (yname=="uPrime"):
        fname=os.path.join(mode, "LEMOS_jetmixer_%s_radial_xD%1.1f.csv"%(msn,xD))
    elif yname=="G" or yname=="gPrime":
        fname=os.path.join(mode, "LEMOS_jetmixer_%s_radial_mixfrac_filtered_xD%1.1f.csv"%(msn,xD))
  elif xname=="xD":
    fname=os.path.join(mode, "LEMOS_jetmixer_%s_axial.csv"%msn)
    
  
  data=np.array([0,0])
  if not xcol is None and not ycol is None:
    data=np.loadtxt(os.path.join(basedir, fname), delimiter=';')[:,(xcol,ycol)]
  if not op is None: 
    data=op(data)

  return data.tolist()
