
import os, sys, re, time, math, zipfile, StringIO
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







def area(mesh, group_ma_name):
  from Cata.cata import CREA_MAILLAGE, AFFE_MODELE, DEFI_MATERIAU, \
      AFFE_MATERIAU, AFFE_CARA_ELEM, POST_ELEM, DETRUIRE
  from Accas import _F
  
  tmpmesh=CREA_MAILLAGE(MAILLAGE=mesh,
			MODI_MAILLE=_F(GROUP_MA=group_ma_name,
                                    OPTION='TRIA6_7',
                                    #PREF_NOEUD='NT'
                                    )
                       );
  
  dummod=AFFE_MODELE(MAILLAGE=tmpmesh,
		    VERIF='MAILLE',
		    AFFE=(_F(GROUP_MA=(group_ma_name),# '1out', '2in'),
			     PHENOMENE='MECANIQUE',
			     MODELISATION='COQUE_3D'),
			  ),
		      );
		      
  dummat=DEFI_MATERIAU(ELAS=_F(E=210000.0, RHO=1, NU=0.0,),);

  dmatass=AFFE_MATERIAU(MAILLAGE=tmpmesh, AFFE=_F(GROUP_MA=group_ma_name, MATER=dummat,),);

  tmpcara=AFFE_CARA_ELEM(MODELE=dummod,
		      COQUE=_F(GROUP_MA=group_ma_name, EPAIS=1.0,),);

  tmptab=POST_ELEM(MASS_INER=_F(GROUP_MA=group_ma_name),
		    CARA_ELEM=tmpcara,
		    TITRE='tit_von_post_elem',
		    MODELE=dummod,
		    CHAM_MATER=dmatass,
		    );
  #IMPR_TABLE(TABLE=tab_post,);
  print tmptab.EXTR_TABLE()
  A = tmptab['MASSE',1]
  DETRUIRE(CONCEPT=(_F(NOM=(tmpmesh, dummod,dummat,dmatass,tmpcara,tmptab))), INFO=1);
  return A






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
  
  allTempRamps={}
  
  """
  some implicit conventions on group naming:
  """
  def __init__(self, label, Ds, Fv, headface, beamhead, nutface, beamnut, steps, E=210000.0, alpha=11e-6):
    from Cata.cata import DEFI_FONCTION
    from Accas import _F
    
    self.label=label
    self.headface=headface
    self.nutface=nutface
    self.beamhead=beamhead
    self.beamnut=beamnut
    self.steps=steps
    self.Ds=Ds
    self.E=E
    self.alpha=alpha
    self.Fv=Fv

    
    self.dt = Fv / (E*(math.pi*0.25*Ds**2)*self.alpha)
    blT=BoltedJoint.allTempRamps
    blT[self.label]=DEFI_FONCTION(
		    NOM_PARA='INST',
		    VALE=(
			    0,          	0,
			    self.steps,      	-self.dt
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
             
  def LIAISON_SOLIDE_HEAD(self):
    from Accas import _F
    return _F(
	      GROUP_NO=(self.beamhead, self.headface),
             )
             
  def LIAISON_RBE3_NUT(self):
    from Accas import _F
    if not self.nutface is None:
      return _F(
		GROUP_NO_MAIT=self.beamnut,
		DDL_MAIT=('DX', 'DY', 'DZ', 'DRX', 'DRY', 'DRZ',),
		GROUP_NO_ESCL=self.nutface,
		DDL_ESCL='DX-DY-DZ',
		COEF_ESCL=1,
	      )
    else: 
      return None

  def DDL_IMPO_NUT(self):
    from Accas import _F
    if self.nutface is None:
      return  _F(GROUP_NO=self.beamnut,
		  DX=0.0,
		  DY=0.0,
		  DZ=0.0,
		  DRX=0.0, DRY=0.0, DRZ=0.0
		  )
    else: 
      return None

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
                        self.steps,   	-self.dt
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




class StressAssessmentPoints:
  
  def __init__(self):
    self.zip_in_memory=StringIO.StringIO()
    self.zip_archive = zipfile.ZipFile(
      self.zip_in_memory, 
      "a", 
      zipfile.ZIP_DEFLATED, 
      False
    )

  def appendFile(self, fname, contents):
    self.zip_archive.writestr(fname, contents)

  def write(self, fname):
    self.zip_archive.close()
    self.zip_in_memory.seek(0)

    f = file(fname, "w")
    f.write(self.zip_in_memory.read())
    f.close()

  def extractMaxBeamStress(self, label, group, s):
    from Cata.cata import POST_RELEVE_T, CALC_TABLE, IMPR_TABLE, DETRUIRE
    from Accas import _F
    
    tabex=POST_RELEVE_T(
      ACTION=tuple( [
	_F (
	  INTITULE=label+':'+qty ,
	  OPERATION='EXTREMA' ,
	  GROUP_MA = group ,
	  RESULTAT=s ,
	  NOM_CHAM='SIPO_ELNO' ,
	  NOM_CMP = ( qty , ) ,
	) for qty in ['SN', 'SVY', 'SVZ', 'SMT', 'SMFY', 'SMFZ'] ] )
    );

    tabmax=CALC_TABLE (
      TABLE=tabex ,
      ACTION = (
	_F (
	  OPERATION='FILTRE' ,
	  NOM_PARA='EXTREMA' ,
	  VALE_K='MAX' ,
	),
      ),
    );
    IMPR_TABLE(TABLE=tabmax);
    
    tabmin=CALC_TABLE (
      TABLE=tabex ,
      ACTION = (
	_F (
	  OPERATION='FILTRE' ,
	  NOM_PARA='EXTREMA' ,
	  VALE_K='MIN' ,
	),
      ),
    );
    IMPR_TABLE(TABLE=tabmin);
    
    extrema={}
    for cm,i in [('SN',1), ('SVY',2), ('SVZ',3), ('SMT',4), ('SMFY',5), ('SMFZ',6)]:
      minv=tabmin['VALE',i]
      maxv=tabmax['VALE',i]
      ex=maxv
      if (abs(minv)>abs(maxv)):
	ex=minv
      extrema[cm]=ex
    
    DETRUIRE( CONCEPT=_F ( NOM= (tabmax,tabmin,tabex) ) , ) ;
    
    snippet="""<?xml version="1.0" encoding="utf-8"?>
<root>
  <selectableSubset name="witnesspointlocation" value="nominalparentmaterial">
    <double name="Szd" value="%g"/>
    <double name="Tsy" value="%g"/>
    <double name="Tsz" value="%g"/>
    <double name="Tt"  value="%g"/>
    <double name="Sby" value="%g"/>
    <double name="Sbz" value="%g"/>
  </selectableSubset>
</root>
"""%(
    extrema['SN'], 
    extrema['SVY'], 
    extrema['SVZ'], 
    extrema['SMT'], 
    extrema['SMFY'], 
    extrema['SMFZ']
    )

    self.appendFile(label+"_stress_values.ist", snippet)



  def extractMaxShellStress(
	self,
	label, 
	group, 
	solInf, solSup
      ):
    
    from Cata.cata import POST_RELEVE_T, CALC_TABLE, IMPR_TABLE, DETRUIRE
    from Accas import _F
    
    critset='SIEQ_ELNO'
    critcmp='VMIS'
    repset='SIEQ_ELNO'
    repcmp=('VMIS', 
	    'PRIN_1', 'PRIN_2', 'PRIN_3', 
	    'VECT_1_X', 'VECT_1_Y', 'VECT_1_Z', 
	    'VECT_2_X', 'VECT_2_Y', 'VECT_2_Z', 
	    'VECT_3_X', 'VECT_3_Y', 'VECT_3_Z')
    
    vmis	=  0.
    sigma123	= [0.,0.,0.]
    pt		= [0.,0.,0.]

    for s in [solInf, solSup]:
      tabmax=POST_RELEVE_T(
	ACTION=_F (
	  INTITULE='extremes' ,
	  OPERATION='EXTREMA' ,
	  GROUP_MA = group ,
	  RESULTAT=s ,
	  NOM_CHAM=critset ,
	  NOM_CMP = ( critcmp , ) ,
	  #LIST_INST = ( liste , ) ,
	),
      );

      tabmax=CALC_TABLE (
	TABLE=tabmax ,
	reuse=tabmax ,
	ACTION = (
	  _F (
	    OPERATION='FILTRE' ,
	    NOM_PARA='EXTREMA' ,
	    VALE_K='MAX' ,
	  ),
	  _F (
	    OPERATION='TRI' ,
	    NOM_PARA='VALE' ,
	    ORDRE='CROISSANT' ,
	  ),
	),
      );

      thiselem = tabmax [ 'MAILLE' , 1 ] ;
      thisnode = tabmax [ 'NOEUD' , 1 ] ;
      valatmax=POST_RELEVE_T(
	ACTION=_F (
	  INTITULE=label ,
	  OPERATION='EXTRACTION' ,
	  RESULTAT=s ,
	  NOM_CHAM=repset ,
	    MAILLE=thiselem ,
	    NOEUD=thisnode,
	    NOM_CMP=repcmp
	),
      );
      IMPR_TABLE(TABLE=valatmax);
      
      cur_vmis=valatmax['VMIS',1]
      
      if (abs(cur_vmis)>vmis):
	vmis     = cur_vmis
	sigma123 = [ valatmax['PRIN_1',1], valatmax['PRIN_2',1], valatmax['PRIN_3',1] ]
	pt       = [ valatmax['COOR_X',1], valatmax['COOR_Y',1], valatmax['COOR_Z',1] ]
      
      DETRUIRE( CONCEPT=_F ( NOM= (tabmax,valatmax) ) , ) ;

    snippet="""<?xml version="1.0" encoding="utf-8"?>
<root>
  <selectableSubset name="witnesspointlocation" value="localparentmaterial">
    <double name="S1" value="%g"/>
    <double name="S2" value="%g"/>
    <double name="S3" value="%g"/>
  </selectableSubset>
</root>
  """%( sigma123[0], sigma123[1], sigma123[2] )
    self.appendFile(label+"_stress_values.ist", snippet)
      
    snippet="""
sphere1 = Sphere()

# Properties modified on sphere1
sphere1.Center = [%g, %g, %g]
sphere1.Radius = 50

for view in GetRenderViews():
 sphere1Display = Show(sphere1, view)
 sphere1Display.DiffuseColor = [1.0, 0.0, 0.0]
"""%(pt[0],pt[1],pt[2])
    self.appendFile(label+"_paraview_asp_marker.py", snippet)

  def extractReactionForces(
	self,
	label, 
	singlenode_groups, 
	cumulnode_groups, 
	s
      ):
    
    from Cata.cata import POST_RELEVE_T, CALC_TABLE, IMPR_TABLE, DETRUIRE
    from Accas import _F

    ops=[]
    ops2=[]
    for gname in singlenode_groups:
      ops.append(_F(OPERATION='EXTRACTION',
                                 INTITULE=gname,
                                 RESULTAT=s,
                                 NOM_CHAM='REAC_NODA',
                                 GROUP_NO=gname,
                                 RESULTANTE=('DX','DY','DZ',
					     'DRX','DRY','DRZ'),)
                                 );   
      ops2.append(_F(OPERATION='EXTRACTION',
				      INTITULE=gname,
				      RESULTAT=s,
				      NOM_CHAM='DEPL',
				      GROUP_NO=gname,
				      NOM_CMP=('DX', 'DY', 'DZ',)
				  )
				);
    for gname,center in cumulnode_groups:
      ops.append(_F(OPERATION='EXTRACTION',
                                 INTITULE=gname,
                                 RESULTAT=s,
                                 NOM_CHAM='REAC_NODA',
                                 GROUP_NO=gname,
                                 RESULTANTE=('DX','DY','DZ',),
                                 MOMENT=('DRX', 'DRY', 'DRZ'),
                                 POINT=center
                                 ),
				);
      
    reac=POST_RELEVE_T(ACTION=tuple(ops));
    deplta=POST_RELEVE_T(ACTION=tuple(ops2));
    
    IMPR_TABLE(TABLE=reac,
           FORMAT='TABLEAU',
           );
    IMPR_TABLE(TABLE=deplta,
           FORMAT='TABLEAU',
           );
    
    csvsnippet="#name,X,Y,Z,FX,FY,FZ,MX,MY,MZ\n" # all in N and mm
    latexsnippet="""\\begin{tabular}{lrrrrrr}
ID & $F_x/N$ & $F_y/N$ & $F_z/N$ & $M_x/Nm$ & $M_y/Nm$ & $M_z/Nm$\\\\
\\hline
"""  ## convert Nmm into Nm!!
    i=0
    for gname in singlenode_groups:
      i+=1
      csvsnippet+="%s,%g,%g,%g,%g,%g,%g,%g,%g,%g\n"%(
	 gname,
	 deplta['COOR_X',i],deplta['COOR_Y',i],deplta['COOR_Z',i],
	 reac['DX',i],reac['DY',i],reac['DZ',i],
	 reac['DRX',i],reac['DRY',i],reac['DRZ',i])
      latexsnippet+="%s & %g & %g & %g & %g & %g & %g\\\\\n"%(
	 gname,
	 reac['DX',i],reac['DY',i],reac['DZ',i],
	 1e-3*reac['DRX',i],1e-3*reac['DRY',i],1e-3*reac['DRZ',i])
       
    for gname,center in cumulnode_groups:
      i+=1
      csvsnippet+="%s,%g,%g,%g,%g,%g,%g,%g,%g,%g\n"%(
	 gname,
	 center[0],center[1],center[2],
	 reac['RESULT_X',i],reac['RESULT_Y',i],reac['RESULT_Z',i],
	 reac['MOMENT_X',i],reac['MOMENT_Y',i],reac['MOMENT_Z',i])
      latexsnippet+="%s & %g & %g & %g & %g & %g & %g\\\\\n"%(
	 gname,
	 reac['RESULT_X',i],reac['RESULT_Y',i],reac['RESULT_Z',i],
	 1e-3*reac['MOMENT_X',i],1e-3*reac['MOMENT_Y',i],1e-3*reac['MOMENT_Z',i])
       
    latexsnippet+="\\end{tabular}\n"

    self.appendFile(label+"_table.csv", csvsnippet)
    self.appendFile(label+"_table.tex", latexsnippet)
    
    DETRUIRE(CONCEPT=_F(NOM=(reac,deplta),),
         INFO=1,);