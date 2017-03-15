#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory

def getToC():
  toc={}
  for Retau in ['180', '395', '590']:
    patx="chan...\\.xcorr.(.+)$"
    files=filter(re.compile(patx).search, os.listdir(os.path.join(basedir, 'chan%s/correlations'%Retau)))
    xcorrs=sorted([re.search(patx, fn).group(1) for fn in files], key=lambda z: float(z))
    patz="chan...\\.zcorr.(.+)$"
    files=filter(re.compile(patz).search, os.listdir(os.path.join(basedir, 'chan%s/correlations'%Retau)))
    zcorrs=sorted([re.search(patz, fn).group(1) for fn in files], key=lambda z: float(z))
    
    toc.update({
      "%s/umean_vs_yp"%Retau: None,
      "%s/wmean_vs_yp"%Retau: None,
      "%s/pmean_vs_yp"%Retau: None,
      "%s/Ruu_vs_yp"%Retau: None,
      "%s/Rvv_vs_yp"%Retau: None,
      "%s/Rww_vs_yp"%Retau: None,
      "%s/Ruv_vs_yp"%Retau: None,
      "%s/Ruw_vs_yp"%Retau: None,
      "%s/Rvw_vs_yp"%Retau: None,
      
      # Two-point correlation along flow direction
      "%s/Ruu_vs_xp@"%Retau: xcorrs,
      "%s/Rvv_vs_xp@"%Retau: xcorrs,
      "%s/Rww_vs_xp@"%Retau: xcorrs,
      "%s/Rpp_vs_xp@"%Retau: xcorrs,
      "%s/Ruv_vs_xp@"%Retau: xcorrs,
      "%s/Rup_vs_xp@"%Retau: xcorrs,
      "%s/Rvp_vs_xp@"%Retau: xcorrs,
      
      # Two-point correlation along spanwise direction
      "%s/Ruu_vs_zp@"%Retau: zcorrs,
      "%s/Rvv_vs_zp@"%Retau: zcorrs,
      "%s/Rww_vs_zp@"%Retau: zcorrs,
      "%s/Rpp_vs_zp@"%Retau: zcorrs,
      "%s/Ruv_vs_zp@"%Retau: zcorrs,
      "%s/Rup_vs_zp@"%Retau: zcorrs,
      "%s/Rvp_vs_zp@"%Retau: zcorrs
      })
  return toc
  
def getProfile(name):
  cp=name.split('/')
  
  Retau=cp[0]
  
  m=re.search("^(.+)_vs_(.+)$", cp[1])
  yname=m.group(1)
  xname=m.group(2)
  
  xcol=None
  ycol=None
  fname=None
  if xname=="yp":
    
    xcol=1
    fname="chan%s/profiles/chan%s.means"%(Retau, Retau)
    if yname=="umean":
      ycol=2
    elif yname=="wmean":
      ycol=4
    elif yname=="pmean":
      ycol=6
    else:
      xcol=1
      fname="chan%s/profiles/chan%s.reystress"%(Retau, Retau)
      if yname=="Ruu":
	ycol=2
      if yname=="Rvv":
	ycol=3
      if yname=="Rww":
	ycol=4
      if yname=="Ruv" or yname=="Rvu":
	ycol=5
      if yname=="Ruw" or yname=="Rwu":
	ycol=6
      if yname=="Rvw" or yname=="Rwv":
	ycol=7
	
  elif xname.startswith("xp@"):
    yp=xname.split('@')[1]
    xcol=1
    
    fname="chan%s/correlations/chan%s.xccorr.%s"%(Retau, Retau, yp)
    if yname=="Ruv":
      ycol=2
    elif yname=="Rup":
      ycol=3
    elif yname=="Rvp":
      ycol=4
    else:
      fname="chan%s/correlations/chan%s.xcorr.%s"%(Retau, Retau, yp)
      if yname=="Ruu":
	ycol=2
      if yname=="Rvv":
	ycol=3
      if yname=="Rww":
	ycol=4
      if yname=="Rpp":
	ycol=5
	
  elif xname.startswith("zp@"):
    yp=xname.split('@')[1]
    xcol=1
    
    fname="chan%s/correlations/chan%s.zccorr.%s"%(Retau, Retau, yp)
    if yname=="Ruv":
      ycol=2
    elif yname=="Rup":
      ycol=3
    elif yname=="Rvp":
      ycol=4
    else:
      fname="chan%s/correlations/chan%s.zcorr.%s"%(Retau, Retau, yp)
      if yname=="Ruu":
	ycol=2
      if yname=="Rvv":
	ycol=3
      if yname=="Rww":
	ycol=4
      if yname=="Rpp":
	ycol=5
  
  data=np.array([[0.0,0.0],[1.0,0.0]])
  if not xcol is None and not ycol is None:
    data=np.loadtxt(os.path.join(basedir, fname))[:,(xcol,ycol)]

  return data.tolist()
