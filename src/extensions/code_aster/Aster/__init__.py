
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
      DETRUIRE(CONCEPT=( _F(NOM=(m[i-1], addmesh) ) ), INFO=1);
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
      

class BoltedJoint(object):
  
  steps=5
  allTempRamps={}
  
  """
  some implicit conventions on group naming:
  """
  def __init__(self, label, Ds, Fv, headface, beamhead, nutface, beamnut, E=210000.0, alpha=11e-6):
    from Cata.cata import DEFI_FONCTION
    from Accas import _F
    
    self.label=label
    self.headface=headface
    self.nutface=nutface
    self.beamhead=beamhead
    self.beamnut=beamnut
    self.Ds=Ds
    self.E=E
    self.alpha=alpha
    self.Fv=Fv

    
    self.dt = Fv / (E*(math.pi*0.25*Ds**2)*self.alpha)
    blT=BoltedJoint.allTempRamps
    blT[self.label]=DEFI_FONCTION(
		    NOM_PARA='INST',
		    VALE=(
			    0,          		0,
			    BoltedJoint.steps,      	-self.dt
			),
		    INTERPOL='LIN',
		    PROL_DROITE='CONSTANT',
		    PROL_GAUCHE='CONSTANT'
		  );


  def MODELE(self):
    from Accas import _F
    return _F(GROUP_MA=self.label,
              PHENOMENE='MECANIQUE',
              MODELISATION='POU_D_T')
              
  def CARA_ELEM(self):
    from Accas import _F
    return _F(GROUP_MA=self.label,
	      SECTION='CERCLE',
	      CARA='R',
	      VALE=self.Ds/2.)

  def LIAISON_RBE3_HEAD(self):
    from Accas import _F
    return _F(
	      GROUP_NO_MAIT=self.beamhead,
	      DDL_MAIT=('DX', 'DY', 'DZ', 'DRX', 'DRY', 'DRZ',),
	      GROUP_NO_ESCL=self.headface,
	      DDL_ESCL='DX-DY-DZ',
	      COEF_ESCL=1,
             )
             
  def LIAISON_RBE3_NUT(self):
    from Accas import _F
    return _F(
	      GROUP_NO_MAIT=self.beamnut,
	      DDL_MAIT=('DX', 'DY', 'DZ', 'DRX', 'DRY', 'DRZ',),
	      GROUP_NO_ESCL=self.nutface,
	      DDL_ESCL='DX-DY-DZ',
	      COEF_ESCL=1,
             )

  def CREA_CHAMP_Temp(self):
    from Accas import _F
    return _F(GROUP_MA=self.label,
	      NOM_CMP='TEMP',
	      VALE_F=BoltedJoint.allTempRamps[self.label]
	      )
	      
  def correctPrestrain(self, sol, inst):
    from Cata.cata import POST_RELEVE_T, DETRUIRE, DEFI_FONCTION
    from Accas import _F

    tabfor=POST_RELEVE_T(ACTION=(_F(OPERATION='EXTRACTION',
                                    INTITULE='forc',
                                    RESULTAT=sol,
                                    NOM_CHAM='SIEF_ELNO',
                                    INST=(inst),
                                    GROUP_NO=(self.beamhead),
                                    NOM_CMP=('N', 'VY', 'VZ', 'MT', 'MFY', 'MFZ',)
                                )
                             ));
                             
    Fax = tabfor['N',1]
    mult = self.Fv/Fax
    self.dt *= mult
    print "Bolt ", self.label,": force Fax = ", Fax, ", resulting multiplier = ", mult

    blT=BoltedJoint.allTempRamps
    
    # re-create temperature ramp for beams
    DETRUIRE(CONCEPT=(
                    _F(NOM=blT[self.label]),
                    _F(NOM=tabfor)
                    ), 
             INFO=1
             );
             
    blT[self.label]=DEFI_FONCTION(NOM_PARA='INST',VALE=(
                        0,       		0,
                        BoltedJoint.steps,   	-self.dt
                       ),
                    INTERPOL='LIN',
                    PROL_DROITE='CONSTANT',
                    PROL_GAUCHE='CONSTANT');
	      
  def evaluate(self, sol, inst_prestressed, inst_fullyloaded):
    from Cata.cata import POST_RELEVE_T, DETRUIRE, DEFI_FONCTION
    from Accas import _F

    data=[]
    for inst in [inst_prestressed, inst_fullyloaded]:
      tabfor=POST_RELEVE_T(ACTION=(_F(OPERATION='EXTRACTION',
				      INTITULE='forc',
				      RESULTAT=sol,
				      NOM_CHAM='SIEF_ELNO',
				      INST=(inst),
				      GROUP_NO=(self.beamhead),
				      NOM_CMP=('N', 'VY', 'VZ', 'MT', 'MFY', 'MFZ',)
				  )
			      ));
      print tabfor.EXTR_TABLE()
      tabuh=POST_RELEVE_T(ACTION=(_F(OPERATION='EXTRACTION',
				      INTITULE='disph',
				      RESULTAT=sol,
				      NOM_CHAM='DEPL',
				      INST=(inst),
				      GROUP_NO=(self.beamhead),
				      NOM_CMP=('DX', 'DY', 'DZ',)
				  )
			      ));
      print tabuh.EXTR_TABLE()
      tabun=POST_RELEVE_T(ACTION=(_F(OPERATION='EXTRACTION',
				      INTITULE='dispn',
				      RESULTAT=sol,
				      NOM_CHAM='DEPL',
				      INST=(inst),
				      GROUP_NO=(self.beamnut),
				      NOM_CMP=('DX', 'DY', 'DZ',)
				  )
			      ));
      
      # get longitudinal direction of screw
      xh=np.array([tabuh['COOR_X',1], tabuh['COOR_Y',1], tabuh['COOR_Z',1]])
      xn=np.array([tabun['COOR_X',1], tabun['COOR_Y',1], tabun['COOR_Z',1]])
      r=xh-xn
      r/=np.linalg.norm(r)

      Fax=tabfor['N',1]
      uh=np.array([tabuh['DX',1], tabuh['DY',1], tabuh['DZ',1]])
      un=np.array([tabun['DX',1], tabun['DY',1], tabun['DZ',1]])
      
      data.append([Fax, uh, un, np.dot(uh, r), np.dot(un, r), np.dot(uh, r)-np.dot(un, r)])
      
      DETRUIRE(CONCEPT=( _F(NOM=(tabfor, tabuh, tabun)) ), INFO=1);
      
    print data
    delta_L=np.linalg.norm(uh-un)
    print "Bolt ", self.label,": force = ", Fax, ", uh=", uh, ", un=", un, ", delta_L=", delta_L
    f_T=np.linalg.norm(data[0][5]) # deformation in unload but prestressed state equals f_T
    F_V=data[0][0] # prestress force
    F_BS=data[1][0]-F_V # maximum screw force
    Delta_f=data[1][5]-data[0][5] # length change between prestressed state and fully loaded state
    delta_S=Delta_f/F_BS # elasticity of screw
    delta_T=f_T/F_V # elasticity of part
    
    return F_V, data[1][0], F_BS, delta_S, delta_T, Delta_f
