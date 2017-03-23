#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory

def getToC():
  toc={
      "Rlam952/k_vs_t": None,
      "Rlam952/enstrophy_vs_t": None,
      "Rlam952/L_vs_t": None,
      "Rlam952/skewness_vs_t": None
      }
  return toc
  
def getProfile(name):
  cp=name.split('/')
  
  Relam=cp[0].lstrip("Rlam")
  
  m=re.search("^(.+)_vs_(.+)$", cp[1])
  yname=m.group(1)
  xname=m.group(2)
  
  xcol=None
  ycol=None
  nskip=0
  fname=None
  if xname=="t":
    xcol=0
    nskip=13
    fname="CB512.f_t"
    if yname=="k":
      ycol=1
    elif yname=="enstrophy":
      ycol=2
    elif yname=="L":
      ycol=3
    elif yname=="skewness":
      ycol=4	
  
  data=np.array([[0.0,0.0],[1.0,0.0]])
  if not xcol is None and not ycol is None:
    data=np.loadtxt(os.path.join(basedir, fname), skiprows=nskip)[:,(xcol,ycol)]

  return data.tolist()
