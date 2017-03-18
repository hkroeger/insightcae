# -*- coding: utf-8 -*-

import math, re
import numpy as np
import scipy.interpolate as interp
import scipy.optimize as opt
from .. import propellerTools as pt

class OpenWaterCurve(object):

    PDmin=0.6
    PDmax=1.4

    kqc=[
            [0.00379368, 0, 0, 0, 0],
            [0.00886523,2,0,0,0],
            [-0.032241,1,1,0,0],
            [0.00344778,0,2,0,0],
            [-0.0408811,0,1,1,0],
            [-0.108009,1,1,1,0],
            [-0.0885381,2,1,1,0],
            [0.188561,0,2,1,0],
            [-0.00370871,1,0,0,1],
            [0.00513696,0,1,0,1],
            [0.0209449,1,1,0,1],
            [0.00474319,2,1,0,1],
            [-0.00723408,2,0,1,1],
            [0.00438388,1,1,1,1],
            [-0.0269403,0,2,1,1],
            [0.0558082,3,0,1,0],
            [0.0161886,0,3,1,0],
            [0.00318086,1,3,1,0],
            [0.015896,0,0,2,0],
            [0.0471729,1,0,2,0],
            [0.0196283,3,0,2,0],
            [-0.0502782,0,1,2,0],
            [-0.030055,3,1,2,0],
            [0.0417122,2,2,2,0],
            [-0.0397722,0,3,2,0],
            [-0.00350024,0,6,2,0],
            [-0.0106854,3,0,0,1],
            [0.00110903,3,3,0,1],
            [-0.000313912,0,6,0,1],
            [0.0035985,3,0,1,1],
            [-0.00142121,0,6,1,1],
            [-0.00383637,1,0,2,1],
            [0.0126803,0,2,2,1],
            [-0.00318278,2,3,2,1],
            [0.00334268,0,6,2,1],
            [-0.00183491,1,1,0,2],
            [0.000112451,3,2,0,2],
            [-0.0000297228,3,6,0,2],
            [0.000269551,1,0,1,2],
            [0.00083265,2,0,1,2],
            [0.00155334,0,2,1,2],
            [0.000302683,0,6,1,2],
            [-0.0001843,0,0,2,2],
            [-0.000425399,0,3,2,2],
            [0.0000869243,3,3,2,2],
            [-0.0004659,0,6,2,2],
            [0.0000554194,1,6,2,2]
            ]

    ktc=[
            [0.00880496,0,0,0,0],
            [-0.204554,1,0,0,0],
            [0.166351,0,1,0,0],
            [0.158114,0,2,0,0],
            [-0.147581,2,0,1,0],
            [-0.481497,1,1,1,0],
            [0.415437,0,2,1,0],
            [0.0144043,0,0,0,1],
            [-0.0530054,2,0,0,1],
            [0.0143481,0,1,0,1],
            [0.0606826,1,1,0,1],
            [-0.0125894,0,0,1,1],
            [0.0109689,1,0,1,1],
            [-0.133698,0,3,0,0],
            [0.00638407,0,6,0,0],
            [-0.00132718,2,6,0,0],
            [0.168496,3,0,1,0],
            [-0.0507214,0,0,2,0],
            [0.0854559,2,0,2,0],
            [-0.0504475,3,0,2,0],
            [0.010465,1,6,2,0],
            [-0.00648272,2,6,2,0],
            [-0.00841728,0,3,0,1],
            [0.0168424,1,3,0,1],
            [-0.00102296,3,3,0,1],
            [-0.0317791,0,3,1,1],
            [0.018604,1,0,2,1],
            [-0.00410798,0,2,2,1],
            [-0.000606848,0,0,0,2],
            [-0.0049819,1,0,0,2],
            [0.0025983,2,0,0,2],
            [-0.000560528,3,0,0,2],
            [-0.00163652,1,2,0,2],
            [-0.000328787,1,6,0,2],
            [0.000116502,2,6,0,2],
            [0.000690904,0,0,1,2],
            [0.00421749,0,3,1,2],
            [0.0000565229,3,6,1,2],
            [-0.00146564,0,3,2,2]
            ]

    def __init__(self):
        pass

    def Kt(self, J, PbyD, AeByA0, z):
        KT = 0.0
        for c in self.ktc:
            KT = KT + \
                c[0] * (J ** c[1]) * (PbyD ** c[2]) * (AeByA0 ** c[3]) * (z ** c[4])
        return KT

    def Kq(self, J, PbyD, AeByA0, z):
        KQ = 0.0
        for c in self.kqc:
            KQ = KQ + \
                c[0] * (J ** c[1]) * (PbyD ** c[2]) * (AeByA0 ** c[3]) * (z ** c[4])
        return KQ

def getToC():
  return pt.getToC()


def getProfile(name):
  return pt.getProfile(OpenWaterCurve(), name)
##  cp=name.split('/')
##
##  if (len(cp)==4):
##    PbyD=float(cp[0])
##    AebyA0=float(cp[1])
##    z=float(cp[2])
##    
##    m=re.search("^(.+)_vs_(.+)$", cp[3])
##    yname=m.group(1)
##    xname=m.group(2)
##    
##    #def eta(J, *args):
##    #  return -(J*owc.Kt(J, PbyD, AebyA0, z)/(2.*math.pi*owc.Kq(J, PbyD, AebyA0, z)))
##    #Jmaxeta=opt.fmin( eta, 0.0)
##    #Js=np.linspace(0, min(1.5,1.25*Jmaxeta), 50)
##    Js=np.linspace(0, 2.0, 50)
##    
##    if xname=="J":
##      if yname=="Kt":
##        return np.array(zip(Js, [owc.Kt(J, PbyD, AebyA0, z) for J in Js])).tolist()
##      if yname=="Kq":
##        return np.array(zip(Js, [owc.Kq(J, PbyD, AebyA0, z) for J in Js])).tolist()
##  elif len(cp)==3:
##    AebyA0=float(cp[0])
##    z=float(cp[1])
##    m=re.search("^(.+)_vs_(.+)$", cp[2])
##    yname=m.group(1)
##    xname=m.group(2)
##    
##    def eta(PbyD, *args):
##      J,=args
##      return -(J*owc.Kt(J, PbyD, AebyA0, z)/(2.*math.pi*owc.Kq(J, PbyD, AebyA0, z)))
##    
##    if xname=="KNTopt" and yname=="Jopt":
##      Js=np.linspace(0, 2.0, 50)
##      result=[]
##      for J in Js:
##        PD=opt.fmin(eta, 1.0, (J,), disp=False)[0]
##        #print PD
##        KNT=(J**4/owc.Kt(J, PD, AebyA0, z))**0.25
##        if np.isnan(KNT): break
##        result.append( [KNT, J] )
##      return result
##    elif xname=="KDTopt" and yname=="Jopt":
##      Js=np.linspace(0, 2.0, 50)
##      result=[]
##      for J in Js:
##        PD=opt.fmin(eta, 1.0, (J,), disp=False)[0]
##        #print PD
##        KDT=(J**4/owc.Kt(J, PD, AebyA0, z))**0.25
##        if np.isnan(KDT): break
##        result.append( [KDT, J] )
##      return result
##    elif xname=="KNPopt" and yname=="Jopt":
##      Js=np.linspace(0, 2.0, 50)
##      result=[]
##      for J in Js:
##        PD=opt.fmin(eta, 1.0, (J,), disp=False)[0]
##        print PD
##        KNP=(J**5/owc.Kq(J, PD, AebyA0, z))**(1./5.)
##        if np.isnan(KNP): break
##        result.append( [KNP, J] )
##      return result
##    elif xname=="KDPopt" and yname=="Jopt":
##      Js=np.linspace(0, 2.0, 50)
##      result=[]
##      for J in Js:
##        PD=opt.fmin(eta, 1.0, (J,), disp=False)[0]
##        #print PD
##        KDP=(J**3/owc.Kq(J, PD, AebyA0, z))**(1./3.)
##        if np.isnan(KDP): break
##        result.append( [KDP, J] )
##      return result
##    elif yname=="PbyD" and xname.startswith("Kt@"):
##      J=float(xname.split('@')[1])
##      PbyDs=np.linspace(owc.PDmin, owc.PDmax, 25)
##      result=[]
##      for PbyD in PbyDs:
##        Kt=owc.Kt(J, PbyD, AebyA0, z)
##        result.append( [Kt, PbyD] )
##      return result
##    elif yname=="PbyD" and xname.startswith("Kq@"):
##      J=float(xname.split('@')[1])
##      PbyDs=np.linspace(owc.PDmin, owc.PDmax, 25)
##      result=[]
##      for PbyD in PbyDs:
##        Kq=owc.Kq(J, PbyD, AebyA0, z)
##        result.append( [Kq, PbyD] )
##      return result
##  return [0.0, 0.0]
  
