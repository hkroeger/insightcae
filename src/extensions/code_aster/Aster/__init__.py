
import os, sys, re, time, math
import numpy as np
import scipy
import scipy.spatial
from scipy.spatial import cKDTree as KDTree
import scipy.interpolate as spi



def getCommDir():
  os.system("cat *.export")
  fn=filter(re.compile(".*\.export").search, os.listdir("."))
  lines=[l.strip() for l in open(fn[0], "r").readlines()]
  pat="^F comm (.+)/([^/]+).comm"
  d=re.search(pat, filter(re.compile(pat).search, lines)[0]).group(1)
  return d



def readMeshes(nMeshes):
  from Cata.cata import LIRE_MAILLAGE, ASSE_MAILLAGE, DETRUIRE
  from Accas import _F
  # read and assemble nMeshes mesh files (names are given in astk)
  m=[None]*nMeshes
  mesh=None
  for i in range(0,nMeshes):
    if i>0: 
      addmesh=LIRE_MAILLAGE(UNITE=20+i, FORMAT='MED');
      m[i]=ASSE_MAILLAGE(
	    MAILLAGE_1=m[i-1],
	    MAILLAGE_2=addmesh,
	    OPERATION='SUPERPOSE');
      DETRUIRE(
	  CONCEPT=( _F(NOM=(m[i-1], addmesh) ) ), 
	  INFO=1
	  );
    else:
      m[i]=LIRE_MAILLAGE(UNITE=20+i, FORMAT='MED');

  return m[nMeshes-1]



def area(group_ma_name):
  pass
  #from Cata.cata import *
  #from Accas import _F

  #elastic=DEFI_MATERIAU(ELAS=_F(E=200000.0, RHO=1, NU=0.0,),);

  #Mat=AFFE_MATERIAU(MAILLAGE=MeshLin, AFFE=_F(TOUT='OUI', MATER=elastic,),);

  #Cara=AFFE_CARA_ELEM(MODELE=dkt,
		      #COQUE=_F(GROUP_MA='PLATE_T1',EPAIS=1.0,),);

  #tab_post=POST_ELEM(MASS_INER=_F(GROUP_MA='PLATE_T1'),
		    #MODELE=dkt,
		    #CHAM_MATER=Mat,
		    #CARA_ELEM=Cara,
		    #TITRE='tit_von_post_elem',);
  #IMPR_TABLE(TABLE=tab_post,);



class PressureField(object):
  def __init__(self, csvfilename, 
	       delimiter=";", 
	       lengthscale=1.0,
	       pressurescale=1e-6 # Pa => MPa
	       ):
      data=np.loadtxt(csvfilename, delimiter=delimiter)
      self.pts=data[:,0:3]*lengthscale
      self.p=data[:,3]*pressurescale
      self.pinterp=spi.NearestNDInterpolator(self.pts, self.p)
        
  def __call__(self, x):
      """
      Interpolate pressure at location x in FEM model
      """
      return self.pinterp(x[0], x[1], x[2])
      

