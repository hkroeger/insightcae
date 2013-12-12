
import os, sys, re, time, math
import numpy as np
import scipy.interpolate as spi



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
  def __init__(self, Ri, phi_ofs=0.0):
    self.Ri=Ri
    self.phi_ofs=phi_ofs
    
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


class ScratchDirComm(object):
  def __init__(self, basepath):
    self.scratchdir=os.path.join(basepath, "scratch")
    print "Using scratchdir: ", self.scratchdir, ", waiting for directory to occur."
    # wait for scratchdir to occur
    while not os.path.exists(self.scratchdir):    
	time.sleep(0.5)
	
class CFDFEMPair(object):
    def __init__(self, scratchdircomm, pairinfo):
        cfdname, femname, trafo = pairinfo
        self.cfdname=cfdname
        self.femname=femname
        self.trafo=trafo
        self.scratchdircomm=scratchdircomm
        self.debug=True
        self.pts=np.loadtxt(os.path.join(self.scratchdircomm.scratchdir, "locations."+self.cfdname), delimiter=";")
        #self.tree=Invdisttree(self.pts)
        
    def reread(self):
        cfd_p=np.loadtxt(os.path.join(self.scratchdircomm.scratchdir, "pressure_values."+self.cfdname), delimiter=";")
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
    
    def writeDisplacements(self, sol, instant):
	from Cata.cata import POST_RELEVE_T, DETRUIRE
	from Accas import _F
    	# extract displacements of coupled boundaries        
	utab=POST_RELEVE_T(ACTION=(_F(OPERATION='EXTRACTION',
					INTITULE='disp',
					RESULTAT=sol,
					NOM_CHAM='DEPL',
					INST=(instant),
					#GROUP_NO=('v_2', 'v_3'),
					GROUP_NO=self.femname,
					TOUT_CMP='OUI'
				    )
				));
	u=utab.EXTR_TABLE().values();
	uinterp_pts=np.array([self.trafo.locationFEMtoCFD(p)
		for p in np.asarray(zip(u['COOR_X'], u['COOR_Y'], u['COOR_Z']))])
	disp=np.array([self.trafo.displacementFEMtoCFD(uinterp_pts[i], u)
		for i,u in enumerate(np.asarray(zip(u['DX'], u['DY'], u['DZ'])))])

	if self.debug:
	  f=open(os.path.join(self.scratchdircomm.scratchdir, "dbg.FEMlocations."+self.cfdname), "w")
	  for p in uinterp_pts: f.write( "%g;%g;%g\n"%(p[0], p[1], p[2]) )
	  f.close()
	  f=open(os.path.join(self.scratchdircomm.scratchdir, "dbg.FEMdisplacements."+self.cfdname), "w")
	  for u in disp: f.write( "%g;%g;%g\n"%(u[0], u[1], u[2]) )
	  f.close()
	
	#uinterp=Invdisttree(uinterp_pts)
	#uinterp.setData(disp)
        #u_pts=[uinterp(p) for p in self.pts]

	uinterp=spi.NearestNDInterpolator(uinterp_pts, disp)
        u_pts=[uinterp(p[0], p[1], p[2]) for p in self.pts]
        
        print "Pair %s/%s: min cmpt="%(self.cfdname,self.femname), \
	      np.min(u_pts), "max cmpt=", np.max(u_pts)
	      
        f=open(os.path.join(self.scratchdircomm.scratchdir, "displacements."+self.cfdname), 'w')
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


def save_remove(fn):
  # otherwise remove will be called twice in parallel runs!
  if os.path.exists(fn): 
    try:
      os.remove(fn)
    except:
      print "Failed to remove", fn

class CFDFEMPairs(object):
  def __init__(self, scratchdircomm, pairinfos, model, globalname):
    self.scratchdircomm=scratchdircomm
    
    from Cata.cata import FORMULE, AFFE_CHAR_MECA_F
    from Accas import _F
    
    self.pairs=[CFDFEMPair(scratchdircomm, pi) for pi in pairinfos]
    
    self.pf=[None]*len(self.pairs)
    for i,p in enumerate(self.pairs):
      p.reread()
      self.pf[i]=FORMULE(NOM_PARA=("X", "Y", "Z"), VALE="%s.pairs[%d]([X, Y, Z])"%(globalname, i))
      
    self.pres=AFFE_CHAR_MECA_F(MODELE=model,
                          PRES_REP=tuple([
                          _F(
                            GROUP_MA=p.femname,
                            PRES=self.pf[i]
                          ) for i,p in enumerate(self.pairs)
                          ])
                         );


  def run(self, runfunc, finalinstant):
    
    class GetOutOfLoop( Exception ):
	pass

    stopfn=os.path.join(self.scratchdircomm.scratchdir, "STOP")

    sol=None

    try:
      while True:

	# wait for all signal files to occur
	present=[False]*len(self.pairs)
	while False in present:
	    if os.path.exists(stopfn): 
		save_remove(stopfn)
		raise GetOutOfLoop()

	    time.sleep(0.5)
	    
	    for i,p in enumerate(self.pairs):
		startfn=os.path.join(self.scratchdircomm.scratchdir, "complete.%s"%p.cfdname)
		if os.path.exists(startfn): 
		    present[i]=True;
		    p.reread()
	
	# do FEM and write results
	sol=runfunc(sol, self.pres)
	for p in self.pairs: 
	  p.writeDisplacements(sol, finalinstant)

	# remove all signal files
	for i,p in enumerate(self.pairs):
	    save_remove(os.path.join(self.scratchdircomm.scratchdir, "complete.%s"%p.cfdname))
	    time.sleep(0.5)

    except GetOutOfLoop:
      pass    
    
    return sol
    
    
