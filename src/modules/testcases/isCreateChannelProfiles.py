#!/usr/bin/env python

import imp, os, sys
import numpy as np
import scipy.interpolate as ip
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-H", "--height", dest="h", metavar='h', default=1.0,
                  help="half-height of the channel")
parser.add_option("-R", "--retau", dest="Retau", metavar='Re_tau', default=395.0,
                  help="friction velocity reynolds number")
(opts, args) = parser.parse_args()

c=imp.load_source('c', os.getenv("HOME")+"/Referenzdaten/MKM_Channel/__init__.py")

def cinterp(x, y):
  xv=list(x)+[1e10]
  yv=list(y)+[y[-1]]
  if (x[0]>0.0):
    xv=[0]+xv
    yv=[y[0]]+yv
  return ip.interp1d(xv, yv, bounds_error=False)

def mirrored(y):
  return list(y)+list(y[::-1])

u_vs_yp=np.array(c.getProfile("%d/umean_vs_yp"%opts.Retau))
xp=u_vs_yp[:,0]
x=-opts.h*(1.-xp/opts.Retau)
x=list(x)+list(-x[::-1])

u=mirrored(u_vs_yp[:,1])

d=np.array(c.getProfile("%d/vmean_vs_yp"%opts.Retau))
v_vs_yp=cinterp(d[:,0], d[:,1])
v=mirrored(v_vs_yp(u_vs_yp[:,0]))

d=np.array(c.getProfile("%d/wmean_vs_yp"%opts.Retau))
w_vs_yp=cinterp(d[:,0], d[:,1])
w=mirrored(w_vs_yp(u_vs_yp[:,0]))

np.savetxt("umean.txt",
  np.transpose(np.vstack([x, u, v, w]))
)

Ruu_vs_yp=np.array(c.getProfile("%d/Ruu_vs_yp"%opts.Retau))
xp=Ruu_vs_yp[:,0]
x=-opts.h*(1.-xp/opts.Retau)
x=list(x)+list(-x[::-1])

Ruu=mirrored(Ruu_vs_yp[:,1])

d=np.array(c.getProfile("%d/Rvv_vs_yp"%opts.Retau))
Rvv_vs_yp=cinterp(d[:,0], d[:,1])
Rvv=mirrored(Rvv_vs_yp(Ruu_vs_yp[:,0]))

d=np.array(c.getProfile("%d/Rww_vs_yp"%opts.Retau))
Rww_vs_yp=cinterp(d[:,0], d[:,1])
Rww=mirrored(Rww_vs_yp(Ruu_vs_yp[:,0]))

np.savetxt("R.txt",
  np.transpose(np.vstack([x, Ruu, Rvv, Rww]))
)

