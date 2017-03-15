#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory

def getToC():
  toc={}
  toc.update({
    # Figure on page 535
    "generic/RuuByUtauSqr_vs_yp": None,
    "generic/RvvByUtauSqr_vs_yp": None,
    "generic/RwwByUtauSqr_vs_yp": None,
    "generic/kPlus_vs_yp": None,
    })
  return toc
  
def getProfile(name):
  cp=name.split('/')
  
  typ=cp[0]
  
  m=re.search("^(.+)_vs_(.+)$", cp[1])
  yname=m.group(1)
  xname=m.group(2)
  
  xcol=None
  ycol=None
  fname=None
  
  if typ=="generic":
    if xname=="yp":
      xcol=0
      fname="universal_reystress_distrib.dat"
      if yname=="RuuByUtauSqr":
	ycol=1
      elif yname=="RwwByUtauSqr":
	ycol=2
      elif yname=="RvvByUtauSqr":
	ycol=3
      elif yname=="kPlus":
	ycol=4
  
  data=np.array([0,0])
  if not xcol is None and not ycol is None:
    data=np.loadtxt(os.path.join(basedir, fname))[:,(xcol,ycol)]

  return data.tolist()
