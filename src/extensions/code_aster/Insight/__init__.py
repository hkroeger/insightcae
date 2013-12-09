
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

#def area():
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

class Transformation(object):
  
  def checkValidity(self):
    pass
    #p=np.array([1,2,3])
    #np.linalg.norm(p - self.
  
  def locationFEMtoCFD(self, pFEM):
    x,y,z=pFEM
    return np.array([x ,y, z])

  def displacementFEMtoCFD(self, pCFD, u):
    x,y,z=pCFD
    return np.array([x, y, z])
  
  
class BearingCFDTransformation(Transformation):
  def __init__(self, Ri, b, phi_ofs=0.0):
    self.Ri=Ri
    self.phi_ofs=phi_ofs
    self.b=b
    
  def locationFEMtoCFD(self, pFEM):
    x,y,z=pFEM
    R=(x**2+y**2)**0.5
    phi=math.atan2(x, -y)+self.phi_ofs
    if phi>math.pi: phi-=2.*math.pi
    if phi<-math.pi: phi+=2.*math.pi
    return np.array([phi*self.Ri, -(R-self.Ri), z])

  def displacementFEMtoCFD(self, pCFD, u):
    x,y,z=pCFD
    phi=x/self.Ri
    ca=math.cos(-phi+self.phi_ofs)
    sa=math.sin(-phi+self.phi_ofs)
    return np.array([ca*u[0]-sa*u[1], sa*u[0]+ca*u[1], u[2]]) # rotation by phi around -z


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
      

class CFDFEMPair(object):
    def __init__(self, cfdname, femname, trafo):
        self.cfdname=cfdname
        self.femname=femname
        self.trafo=trafo
        self.pts=np.loadtxt(os.path.join(scratchdir, "locations."+self.cfdname), delimiter=";")
        #self.tree=Invdisttree(self.pts)
        
    def reread(self):
        cfd_p=np.loadtxt(os.path.join(scratchdir, "pressure_values."+self.cfdname), delimiter=";")
        #self.tree.setData(cfd_p)
        self.pinterp=spi.NearestNDInterpolator(self.pts, cfd_p)
        
    def __call__(self, x):
	"""
	Interpolate pressure at location x in FEM model
	"""
        #r=self.tree(self.trafo.locationFEMtoCFD(x))
        p=self.trafo.locationFEMtoCFD(x)
        r=self.pinterp(p[0], p[1], p[2])
        #print r
        return r
    
    def writeDisplacements(self, sol, instant=2):
    	# extract displacements of coupled boundaries        
	utab=POST_RELEVE_T(ACTION=(_F(OPERATION='EXTRACTION',
					INTITULE='disp',
					RESULTAT=sol,
					NOM_CHAM='DEPL',
					INST=(instant),
					GROUP_NO=('v_2', 'v_3'),
					#GROUP_NO=self.femname,
					TOUT_CMP='OUI'
				    )
				));
	u=utab.EXTR_TABLE().values();
	uinterp_pts=np.array([self.trafo.locationFEMtoCFD(p)
		for p in np.asarray(zip(u['COOR_X'], u['COOR_Y'], u['COOR_Z']))])
	disp=np.array([self.trafo.displacementFEMtoCFD(uinterp_pts[i], u)
		for i,u in enumerate(np.asarray(zip(u['DX'], u['DY'], u['DZ'])))])

	if debug:
	  f=open(os.path.join(scratchdir, "dbg.FEMlocations."+self.cfdname), "w")
	  for p in uinterp_pts: f.write( "%g;%g;%g\n"%(p[0], p[1], p[2]) )
	  f.close()
	  f=open(os.path.join(scratchdir, "dbg.FEMdisplacements."+self.cfdname), "w")
	  for u in disp: f.write( "%g;%g;%g\n"%(u[0], u[1], u[2]) )
	  f.close()
	
	#uinterp=Invdisttree(uinterp_pts)
	#uinterp.setData(disp)
        #u_pts=[uinterp(p) for p in self.pts]

	uinterp=spi.NearestNDInterpolator(uinterp_pts, disp)
        u_pts=[uinterp(p[0], p[1], p[2]) for p in self.pts]
        
        print "Pair %s/%s: min cmpt="%(self.cfdname,self.femname), \
	      np.min(u_pts), "max cmpt=", np.max(u_pts)
	      
        f=open(os.path.join(scratchdir, "displacements."+self.cfdname), 'w')
        f.write("(\n")
        for j,u in enumerate(u_pts): 
	  f.write("(%g %g %g)\n"%(u[0], u[1], u[2]))
        f.write(")\n")
        f.flush()
        os.fsync(f.fileno())
        f.close()
        
	DETRUIRE(
         CONCEPT=( _F(NOM=(utab) ) ), 
         INFO=1
         );
