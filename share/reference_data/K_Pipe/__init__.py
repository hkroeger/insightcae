#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory


def getToC():
  toc={}
  for Retau in ['180']:
    #pat="chan...\\.xcorr.(.+)$"
    #files=filter(re.compile(pat).search, os.listdir(os.path.join(basedir, 'chan%s/correlations'%Retau)))
    #xcorrs=[re.search(pat, fn).group(1) for fn in files]
    ycorrs=['5', '10', '20', '40', '80']
    toc.update({
      
      # mean profiles
      "%s/urmean_vs_yp"%Retau: None,
      "%s/uphimean_vs_yp"%Retau: None,
      "%s/uzmean_vs_yp"%Retau: None,
      "%s/pmean_vs_yp"%Retau: None,
      
      # Reynolds stresses
      "%s/Rrr_vs_yp"%Retau: None,
      "%s/Rphiphi_vs_yp"%Retau: None,
      "%s/Rzz_vs_yp"%Retau: None,
      "%s/Rrphi_vs_yp"%Retau: None,
      "%s/Rrz_vs_yp"%Retau: None,
      "%s/Rphiz_vs_yp"%Retau: None,
      
      # axial correlation profiles
      "%s/Rrr_vs_zp@"%Retau: ycorrs,
      "%s/Rphiphi_vs_zp@"%Retau: ycorrs,
      "%s/Rzz_vs_zp@"%Retau: ycorrs,
      "%s/Rpp_vs_zp@"%Retau: ycorrs,

      # azimuthal correlation profiles
      "%s/Rrr_vs_rphip@"%Retau: ycorrs,
      "%s/Rphiphi_vs_rphip@"%Retau: ycorrs,
      "%s/Rzz_vs_rphip@"%Retau: ycorrs,
      "%s/Rpp_vs_rphip@"%Retau: ycorrs
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
  
  sqr_y=lambda data: np.array(zip(data[:,0], data[:,1]**2))
  
  if xname=="yp":
    
    xcol=0
    fname="Mean_Ur_Uphi_Uz_P.dat"
    if yname=="urmean":
      xcol=1
      ycol=2
    elif yname=="uphimean":
      ycol=3
    elif yname=="uzmean":
      ycol=4
    elif yname=="pmean":
      ycol=5
    else:
      xcol=0
      fname="RMS_ur_uphi_uz_p.dat"
      if yname=="Rrr":
	xcol=1
	ycol=2
	op=sqr_y
      elif yname=="Rphiphi":
	ycol=3
	op=sqr_y
      elif yname=="Rzz":
	ycol=4
	op=sqr_y
      else:
	xcol=1
	fname="Reynolds.dat"
	if yname=="Rrphi" or yname=="Rphir":
	  ycol=3
	if yname=="Rzphi" or yname=="Rphiz":
	  xcol=0
	  ycol=4
	if yname=="Rrz" or yname=="Rzr":
	  ycol=5
	
  elif xname.startswith("zp@"):
    zp=xname.split('@')[1]
    xcol=0
    
    fname="TwoPointCorrel_u_p_longi_yp%s.dat"%(zp)
    if yname=="Rrr":
      ycol=1
    elif yname=="Rphiphi":
      ycol=2
    elif yname=="Rzz":
      ycol=3
    elif yname=="Rpp":
      ycol=4
      
  elif xname.startswith("rphip@"):
    zp=xname.split('@')[1]
    xcol=0
    
    fname="TwoPointCorrel_u_p_azi_yp%s.dat"%(zp)
    if yname=="Rrr":
      xcol=1
      ycol=2
    elif yname=="Rphiphi":
      ycol=3
    elif yname=="Rzz":
      ycol=4
    elif yname=="Rpp":
      ycol=5
  
  data=np.array([0,0])
  if not xcol is None and not ycol is None:
    data=np.loadtxt(os.path.join(basedir, fname))[:,(xcol,ycol)]
  if not op is None: 
    data=op(data)

  return data.tolist()
