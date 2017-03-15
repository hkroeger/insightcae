#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory

ycorrs=['3.458', '21.99', '122', '169.1']

def getToC():
  toc={}
  for Retau in ['180']:
    #pat="chan...\\.xcorr.(.+)$"
    #files=filter(re.compile(pat).search, os.listdir(os.path.join(basedir, 'chan%s/correlations'%Retau)))
    #xcorrs=[re.search(pat, fn).group(1) for fn in files]

    toc.update({
      
      # mean profiles
      "%s/uzmean_vs_yp"%Retau: None,
      
      # Reynolds stresses
      "%s/Rrr_vs_yp"%Retau: None,
      "%s/Rphiphi_vs_yp"%Retau: None,
      "%s/Rzz_vs_yp"%Retau: None,
      "%s/Rrz_vs_yp"%Retau: None,
      
      # axial correlation profiles
      "%s/Rrr_vs_zp@"%Retau: ycorrs,
      "%s/Rphiphi_vs_zp@"%Retau: ycorrs,
      "%s/Rzz_vs_zp@"%Retau: ycorrs,
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
  op=None
  
  yp_x=lambda data: np.array(zip((1.-data[:,0])*180., data[:,1]))
  zp_x=lambda data: np.array(zip(2.*data[:,0]*180., data[:,1]))
  sqr_y=lambda data: np.array(zip(data[:,0], data[:,1]**2))
  sqr_y_yp_x=lambda data: np.array(zip((1.-data[:,0])*180., data[:,1]**2))
  
  if xname=="yp":
    
    xcol=0
    fname="PCH00_01"
    if yname=="uzmean":
      ycol=1
      op=yp_x
    elif yname=="Rrr":
      ycol=3
      op=sqr_y_yp_x
    elif yname=="Rphiphi":
      ycol=4
      op=sqr_y_yp_x
    elif yname=="Rzz":
      ycol=5
      op=sqr_y_yp_x
    elif yname=="Rrz" or yname=="Rzr":
      ycol=2
      op=sqr_y_yp_x
	
  elif xname.startswith("zp@"):
    zp=xname.split('@')[1]
    xcol=0
    
    fname="PCH00_13"
    ri=ycorrs.index(zp)
    c0=1+ri*3
    if yname=="Rrr":
      ycol=c0+0
      op=zp_x
    elif yname=="Rphiphi":
      ycol=c0+1
      op=zp_x
    elif yname=="Rzz":
      ycol=c0+2
      op=zp_x
  
  data=np.array([0,0])
  if not xcol is None and not ycol is None:
    data=np.loadtxt(os.path.join(basedir, fname))[:,(xcol,ycol)]
  if not op is None: 
    data=op(data)

  return data.tolist()
