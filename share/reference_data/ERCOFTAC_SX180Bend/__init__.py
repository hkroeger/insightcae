#!/usr/bin/python

import re
import numpy as np

import inspect, os
basedir=os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))) # script directory


def getToC():
  toc={
    "in/umean_vs_x_y": None,
    "in/vmean_vs_x_y": None,
    "in/wmean_vs_x_y": None,
    "in/tke_vs_x_y": None,
    "in/epsilon_vs_x_y": None
  }

  for theta in ['0', '45', '90', '135', '180']:
    toc.update({
      
      # mean profiles
      "%s/umeanbywb_vs_x_y"%theta: None,
      "%s/vmeanbywb_vs_x_y"%theta: None,
      "%s/wmeanbywb_vs_x_y"%theta: None,
      
      # Reynolds stresses
      "%s/Ruu_vs_x_y"%theta: None,
      "%s/Rvv_vs_x_y"%theta: None,
      "%s/Rww_vs_x_y"%theta: None,
      "%s/Ruv_vs_x_y"%theta: None,
      "%s/Ruw_vs_x_y"%theta: None,
      "%s/Rvw_vs_x_y"%theta: None,
      })

  return toc
  
def getProfile(name):
  cp=name.split('/')
  
  theta=cp[0]
  m=re.search("^(.+)_vs_(.+)_(.+)$", cp[1])
  zname=m.group(1)
  xname=m.group(2)
  yname=m.group(3)

  data=np.array([0,0])

  if (theta=="in"):
    if (zname=="umean"):
     zcol=2
    elif (zname=="vmean"):
     zcol=3
    elif (zname=="wmean"):
     zcol=4
    elif (zname=="tke"):
     zcol=5
    elif (zname=="epsilon"):
     zcol=6
    else:
     raise Exception("Unknown quantity in inlet section: "+zname)

    data=np.loadtxt(os.path.join(basedir, "input.translated"))[:,(0,1,zcol)]
  else:
    pass
  

  return data.tolist()
