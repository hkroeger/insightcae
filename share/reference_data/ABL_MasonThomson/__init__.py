#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory

def getToC():
  toc={}
  for case in ['B10']:
    toc.update({
      "%s/umean_vs_zp"%case: None,
      "%s/vmean_vs_zp"%case: None,
      "%s/Ruu_vs_zp"%case: None,
      "%s/Rvv_vs_zp"%case: None,
      "%s/Rww_vs_zp"%case: None,
      "%s/Ruv_vs_zp"%case: None,
      })
  return toc
  
def getProfile(name):
  cp=name.split('/')
  
  case=cp[0]
  
  m=re.search("^(.+)_vs_(.+)$", cp[1])
  yname=m.group(1)
  xname=m.group(2)
  
  xfac=1.
  yfac=1.
  Ug=10.
  f=1e-4
  z0=0.1
  nu=1.71e-5
  if case=="B10":
    utau=( 1.72 * 1e-3 * Ug**2)**0.5
    if (yname in ['Ruu','Rvv','Rww','Ruv']):
      xfac=1000.*utau/nu
      yfac=1./utau**2
    elif (yname in ['umean', 'vmean']):
      xfac=utau/nu
      yfac=1./utau
    else:
      raise 'unknown profile identifier'
  else:
    raise 'unknown case'
       
  
  xcol=None
  ycol=None
  fname=None
  if xname=="zp":
    
    xcol=0
    ycol=1
    fname="%s_%s.csv"%(case, yname)	
  
  data=np.array([[0.0,0.0],[1.0,0.0]])
  if not xcol is None and not ycol is None:
    df=np.loadtxt(os.path.join(basedir, fname))
    data=np.c_[df[:,xcol]*xfac, df[:,ycol]*yfac] 

  return data.tolist()
