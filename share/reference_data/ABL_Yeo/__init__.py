#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory

def getToC():
  toc={}
  for case in ['Yeo']:
    toc.update({
      "%s/umean_vs_zp"%case: None,
      "%s/Ruu_vs_zp"%case: None,
      "%s/Rvv_vs_zp"%case: None,
      "%s/Rww_vs_zp"%case: None,
      "%s/Ruw_vs_zp"%case: None,
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
  H=10.
  uref=30.
  zref=10.
  z0=0.0003
  nu=1.455e-5
  utau=1.181
  fname=None
  if case=="Yeo":
    if (yname in ['Ruu','Rvv','Rww','Ruw']):
      xfac=H*utau/nu
      yfac=1.
      fname="%sByUtauSq_vs_zByH.csv"%(yname)
    elif (yname in ['umean']):
      fname="%s_vs_zByH.csv"%(yname)
      xfac=H*utau/nu
      yfac=1./utau
    else:
      raise 'unknown profile identifier'
  else:
    raise 'unknown case'
       
  xcol=None
  ycol=None
  xcol=0
  ycol=1
  
  data=np.array([[0.0,0.0],[1.0,0.0]])
  if not xcol is None and not ycol is None:
    df=np.loadtxt(os.path.join(basedir, fname))
    data=np.c_[df[:,xcol]*xfac, df[:,ycol]*yfac] 

  return data.tolist()
