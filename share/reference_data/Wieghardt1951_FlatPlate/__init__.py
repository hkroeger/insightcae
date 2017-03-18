#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory

def getToC():
  toc={}
  toc.update({
    "u17.8/cf_vs_x": None,
    "u17.8/delta1_vs_x": None,
    "u17.8/delta2_vs_x": None,
    "u17.8/delta3_vs_x": None
    })
  return toc
  
def getProfile(name):
  cp=name.split('/')
  
  sel=cp[0]
  
  m=re.search("^(.+)_vs_(.+)$", cp[1])
  yname=m.group(1)
  xname=m.group(2)
  
  xcol=None
  ycol=None
  fname=None
  data=None
  
  if xname=="x":
    xcol=0
    ycol=1
    fname="wieghardt1951_fig1_"+sel+"_"+cp[1]+".csv"
    try:
      data=np.loadtxt(os.path.join(basedir, fname))[:,(xcol,ycol)]
    except:
      data=np.array([0,0])
      
  return data.tolist()
