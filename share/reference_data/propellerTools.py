# -*- coding: utf-8 -*-

import math, re
import numpy as np
import scipy.interpolate as interp
import scipy.optimize as opt

def getToC():
    return {
    "PbyD/AebyA0/z/Kt_vs_J": None,
    "PbyD/AebyA0/z/Kq_vs_J": None,
    "AebyA0/z/Jopt_vs_KNTopt": None,
    "AebyA0/z/Jopt_vs_KDTopt": None,
    "AebyA0/z/Jopt_vs_KNPopt": None,
    "AebyA0/z/Jopt_vs_KDPopt": None,
    "AebyA0/z/PbyD_vs_Kt@": [],
    "AebyA0/z/PbyD_vs_Kq@": []
    }
    
def getProfile(owc, name):
  cp=name.split('/')
  if (len(cp)==4):
    PbyD=float(cp[0])
    AebyA0=float(cp[1])
    z=float(cp[2])
    
    m=re.search("^(.+)_vs_(.+)$", cp[3])
    yname=m.group(1)
    xname=m.group(2)
    
    #def eta(J, *args):
    #  return -(J*owc.Kt(J, PbyD, AebyA0, z)/(2.*math.pi*owc.Kq(J, PbyD, AebyA0, z)))
    #Jmaxeta=opt.fmin( eta, 0.0)
    #Js=np.linspace(0, min(1.5,1.25*Jmaxeta), 50)
    Js=np.linspace(0, 2.0, 50)
    
    if xname=="J":
      if yname=="Kt":
        return np.array(zip(Js, [owc.Kt(J, PbyD, AebyA0, z) for J in Js])).tolist()
      if yname=="Kq":
        return np.array(zip(Js, [owc.Kq(J, PbyD, AebyA0, z) for J in Js])).tolist()
  elif len(cp)==3:
    AebyA0=float(cp[0])
    z=float(cp[1])
    m=re.search("^(.+)_vs_(.+)$", cp[2])
    yname=m.group(1)
    xname=m.group(2)
    
    def eta(PbyD, *args):
      J,=args
      return -(J*owc.Kt(J, PbyD, AebyA0, z)/(2.*math.pi*owc.Kq(J, PbyD, AebyA0, z)))
    
    if xname=="KNTopt" and yname=="Jopt":
      Js=np.linspace(0, 2.0, 50)
      result=[]
      for J in Js:
        PD=opt.fmin(eta, 1.0, (J,), disp=False)[0]
        #print PD
        KNT=(J**4/owc.Kt(J, PD, AebyA0, z))**0.25
        if np.isnan(KNT): break
        result.append( [KNT, J] )
      return result
    elif xname=="KDTopt" and yname=="Jopt":
      Js=np.linspace(0, 2.0, 50)
      result=[]
      for J in Js:
        PD=opt.fmin(eta, 1.0, (J,), disp=False)[0]
        #print PD
        KDT=(J**4/owc.Kt(J, PD, AebyA0, z))**0.25
        if np.isnan(KDT): break
        result.append( [KDT, J] )
      return result
    elif xname=="KNPopt" and yname=="Jopt":
      Js=np.linspace(0, 2.0, 50)
      result=[]
      for J in Js:
        PD=opt.fmin(eta, 1.0, (J,), disp=False)[0]
        print PD
        KNP=(J**5/owc.Kq(J, PD, AebyA0, z))**(1./5.)
        if np.isnan(KNP): break
        result.append( [KNP, J] )
      return result
    elif xname=="KDPopt" and yname=="Jopt":
      Js=np.linspace(0, 2.0, 50)
      result=[]
      for J in Js:
        PD=opt.fmin(eta, 1.0, (J,), disp=False)[0]
        #print PD
        KDP=(J**3/owc.Kq(J, PD, AebyA0, z))**(1./3.)
        if np.isnan(KDP): break
        result.append( [KDP, J] )
      return result
    elif yname=="PbyD" and xname.startswith("Kt@"):
      J=float(xname.split('@')[1])
      PbyDs=np.linspace(owc.PDmin, owc.PDmax, 25)
      result=[]
      for PbyD in PbyDs:
        Kt=owc.Kt(J, PbyD, AebyA0, z)
        result.append( [Kt, PbyD] )
      return result
    elif yname=="PbyD" and xname.startswith("Kq@"):
      J=float(xname.split('@')[1])
      PbyDs=np.linspace(owc.PDmin, owc.PDmax, 25)
      result=[]
      for PbyD in PbyDs:
        Kq=owc.Kq(J, PbyD, AebyA0, z)
        result.append( [Kq, PbyD] )
      return result
  return [0.0, 0.0]