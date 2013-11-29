 #!/usr/bin/python

import math, os, sys, subprocess
import numpy as np
import numpy.linalg as npla
from OCC.Utils.Topology import Topo
from OCC.TopTools import TopTools_IndexedMapOfShape, TopTools_ListOfShape, TopTools_ListIteratorOfListOfShape
from OCC.BRep import BRep_Builder, BRep_Tool
from OCC.TopExp import TopExp_Explorer, TopExp
from OCC.TopoDS import TopoDS_Shape, TopoDS_Shape, TopoDS
from OCC.BRepTools import BRepTools
from OCC.GeomAdaptor import GeomAdaptor_Surface, GeomAdaptor_Curve
import OCC.GeomAbs as GeomAbs
from OCC.gp import gp, gp_Pnt, gp_Ax1, gp_Dir, gp_Trsf, gp_Circ, gp_Pln, gp_Ax2, gp_Ax3, gp_Cylinder, \
    gp_Vec, gp_XYZ
from OCC.BRepGProp import BRepGProp, BRepGProp_Face, BRepGProp_VolumeProperties, BRepGProp_SurfaceProperties
from OCC.GProp import GProp_GProps
from OCC.STEPControl import STEPControl_Reader

SMALL=1e-6

def toArray(pnt):
  return np.array([pnt.X(), pnt.Y(), pnt.Z()])
  
def toList(tul):
  return [i for i,v in tul]

def toSet(tul):
  return set(toList(tul))

def faceCoG(face):
  props = GProp_GProps()
  BRepGProp.SurfaceProperties(face, props)
  p=props.CentreOfMass()
  return np.array([p.X(), p.Y(), p.Z()])
    
def cylFaceRadius(face):
  adapt=GeomAdaptor_Surface(BRep_Tool.Surface(face))
  if adapt.GetType()==GeomAbs.GeomAbs_Cylinder:
    icyl=adapt.Cylinder()
    return icyl.Radius()
  else:
    raise Exception("Given face is not a cylinder!")
    
def faceArea(face):
  p=GProp_GProps()
  BRepGProp.SurfaceProperties(face, p)
  return p.Mass()

def loadBREP(filename):
  builder=BRep_Builder()
  shape=TopoDS_Shape()
  ok =  BRepTools().Read(shape, filename, builder);
  if not ok: raise Exception("Could not load file "+filename)
  return shape

def loadSTEP(filename):
  stepReader=STEPControl_Reader()
  stepReader.ReadFile(filename);
  stepReader.TransferRoots();
  return stepReader.OneShape()

class equalComp(object):
  def __init__(self, comp, val, deviation=SMALL):
    self.val=val
    self.comp=comp
    self.deviation=deviation
    
  def fulfilled(self, d):
    vec=d[0]
    if abs(vec[self.comp]-self.val)<self.deviation: return True
    return False

class nequalComp(object):
  def __init__(self, comp, val, deviation=SMALL):
    self.val=val
    self.comp=comp
    self.deviation=deviation
    
  def fulfilled(self, d):
    vec=d[0]
    if abs(vec[self.comp]-self.val)>self.deviation: return True
    return False
  
class normCircAligned(object):
  def __init__(self, center, axis, 
	       sense=None,  # or 'right' or 'left'
	       deviation=SMALL):
    self.center=center
    self.sense=sense
    self.deviation=deviation
    self.axis=axis/npla.norm(axis)
    
  def fulfilled(self, d):
    vec=d[0]
    loc=d[1]
    dr=loc-self.center
    dr-=self.axis*np.dot(dr, self.axis)
    dr/=npla.norm(dr)
    du=np.cross(self.axis, dr)
    p=np.dot(du, vec)
    if not self.sense:
      return abs(abs(p)-1.)<self.deviation
    else:
      if self.sense=='right':
	return abs(p-1.)<self.deviation
      elif self.sense=='left':
	return abs(p+1.)<self.deviation
      else:
	raise Exception("undefined sense: "+self.sense+"!")
	return False

	
class vecRadialAligned(object):
  def __init__(self, center, axis, 
	       deviation=SMALL):
    self.center=center
    self.deviation=deviation
    self.axis=axis/npla.norm(axis)
    
  def fulfilled(self, d):
    vec=d[0]
    loc=d[1]
    dr=loc-self.center
    dr-=self.axis*np.dot(dr, self.axis)
    dr/=npla.norm(dr)
    du=np.cross(self.axis, dr)
    p=np.dot(du, vec)
    if not self.sense:
      return abs(abs(p)-1.)<self.deviation
    else:
      if self.sense=='right':
	return abs(p-1.)<self.deviation
      elif self.sense=='left':
	return abs(p+1.)<self.deviation
      else:
	raise Exception("undefined sense: "+self.sense+"!")
	return False
     
   

class vecAligned(object):
  def __init__(self, vec, sense=True, deviation=SMALL):
    self.vec=vec
    self.sense=sense
    self.deviation=deviation
    
  def fulfilled(self, d):
    vec=d[0]
    if self.sense:
      return abs(np.dot(vec, self.vec)-1.) < self.deviation
    else:
      return abs(abs(np.dot(vec, self.vec))-1.) < self.deviation
  
class AND(object):
  def __init__(self, o1, o2):
    self.o1=o1
    self.o2=o2
  def fulfilled(self, d):
    return self.o1.fulfilled(d) and self.o2.fulfilled(d)
    

class SolidBody(object):
  
  def __init__(self, brepfilename):
    self.filename=brepfilename
    self.name=os.path.splitext(os.path.basename(self.filename))[0]
    if (brepfilename.endswith('.brep')):
      self.shape=loadBREP(self.filename)
    elif (brepfilename.endswith('.stp') or brepfilename.endswith('.step')):
      self.shape=loadSTEP(self.filename)
    
    # Generate entity numbering like in gmsh
    self.fmap=TopTools_IndexedMapOfShape()
    self.emap=TopTools_IndexedMapOfShape()
    self.vmap=TopTools_IndexedMapOfShape()
    self.somap=TopTools_IndexedMapOfShape()
    self.shmap=TopTools_IndexedMapOfShape()
    self.wmap=TopTools_IndexedMapOfShape()
    
    for solid in Topo(self.shape).solids():
	if(self.somap.FindIndex(solid) < 1):
	    self.somap.Add(solid)
    
	    for shell in Topo(solid).shells():
		if(self.shmap.FindIndex(shell) < 1):
		    self.shmap.Add(shell)
    
		    for face in Topo(shell).faces():
			if(self.fmap.FindIndex(face) < 1):
			    self.fmap.Add(face)
    
			    for wire in Topo(face).wires():
				if(self.wmap.FindIndex(wire) < 1):
				    self.wmap.Add(wire)
    
				    for edge in Topo(wire).edges():
					if(self.emap.FindIndex(edge) < 1):
					    self.emap.Add(edge)
    
					    for vertex in Topo(edge).vertices():
						if(self.vmap.FindIndex(vertex) < 1):
						    self.vmap.Add(vertex)

    self.namedVertices={}
    self.namedEdges={}
    self.namedFaces={}
    
  def __str__(self):
    s="PART "+self.name+"\nDEFINED FEATURES\n================\n\n"
    
    if len(self.namedEdges)>0:
      s+="Edges\n=====\n"
      for en, es in self.namedEdges.items():
	s+="%s\t:\t%s\n"%(en,es)
      
      s+="\n\n"
      
    if len(self.namedFaces)>0:
      s+="Faces\n=====\n"
      for fn, fs in self.namedFaces.items():
	s+="%s\t:\t%s\n"%(fn,fs)

    return s
    
    
  def nameEdges(self, name, edges):
    if not name in self.namedEdges: self.namedEdges[name]=set()
    self.namedEdges[name].update(edges)

  def nameFaces(self, name, faces):
    if not name in self.namedFaces: self.namedFaces[name]=set()
    self.namedFaces[name].update(faces)
    
  def faceByName(self, name):
    return self.fmap(single(self.namedFaces[name]))
    
  def edgesOfFace(self, face):
    edges=set()
    for e in Topo(face).edges():
      ei=self.emap.FindIndex(e)
      edges.add(ei)
    return edges
    
  def verticesOfEdge(self, edge):
    vertices=set()
    for v in Topo(edge).vertices():
      vi=self.vmap.FindIndex(v)
      vertices.add(vi)
    return vertices

  def verticesOfFace(self, face):
    vertices=set()
    for v in Topo(face).vertices():
      vi=self.vmap.FindIndex(v)
      vertices.add(vi)
    return vertices

  def faceFaces(self, fi):
    rfaces=set()
    org_edges=self.edgesOfFace(TopoDS.face(self.fmap(fi)))
    for fi in range(1, self.fmap.Extent()+1):
      face=TopoDS.face(self.fmap(fi))
      edges=self.edgesOfFace(face)
      if len(edges & org_edges)>0: rfaces.add(fi)
    return rfaces
    
    
  def findCircleEdges(self, filter_d=None, deviation=SMALL):
    edges=set()
    for ei in range(1, self.emap.Extent()+1):
      e,first,last=BRep_Tool.Curve(TopoDS.edge(self.emap(ei)))
      adapt=GeomAdaptor_Curve(e)
      print ei, adapt.GetType()
      if adapt.GetType()==GeomAbs.GeomAbs_Circle:
	icir=adapt.Circle()
	print "%d: circle"%ei, "r=", icir.Radius()
	
	if not filter_d is None:
	  if abs(icir.Radius()-0.5*filter_d) < deviation:
	    edges.add(ei)
	    
	else:
	  edges.add(ei)
	  
    return edges


  def findCylindricalFaces(self, 
			   filter_d=None, 
			   filter_outside=None, 
			   filter_inside=None,
			   filter_center=None,
			   filter_axis=None,
			   deviation=SMALL):
    faces=set()
    for fi in range(1, self.fmap.Extent()+1):
      face=TopoDS.face(self.fmap(fi))
      adapt=GeomAdaptor_Surface(BRep_Tool.Surface(face))
      if adapt.GetType()==GeomAbs.GeomAbs_Cylinder:
	icyl=adapt.Cylinder()
	iax=icyl.Axis()
	#print "%d: cylinder"%fi, "r=", icyl.Radius()
	
	ok=True
	if not filter_d is None:
	  if not abs(icyl.Radius()-0.5*filter_d) < deviation: ok=False	
	  
	if (not filter_axis is None):
	  if not filter_axis.fulfilled( (toArray(iax.Direction()),toArray(icyl.Location())) ): ok=False

	if (not filter_outside is None) or (not filter_inside is None):
	  prop=BRepGProp_Face(face)
	  u1,u2,v1,v2=prop.Bounds()
	  u = (u1+u2)/2;
	  v = (v1+v2)/2;
	  vec=gp_Vec()
	  pnt=gp_Pnt()
	  prop.Normal(u,v,pnt,vec)
	  vec.Normalize()
	  dp=pnt.XYZ()-icyl.Location().XYZ()
	  ax=iax.Direction().XYZ()
	  ax.Normalize()
	  dr=dp-ax.Multiplied(dp.Dot(ax))
	  dr.Normalize()
	  
	  #print "[", pnt.X(), pnt.Y(), pnt.Z(), "]  [", dr.X(), dr.Y(), dr.Z(), "]  [", vec.X(), vec.Y(), vec.Z(), "]", vec.XYZ().Dot(dr)
	  if not filter_outside is None:
	    if filter_outside:
	      if not abs(vec.XYZ().Dot(dr) - 1.) < deviation: ok=False
	  if not filter_inside is None:
	    if filter_inside:
	      if not abs(vec.XYZ().Dot(dr) + 1.) < deviation: ok=False
	      
	  if not filter_center is None:
	    if not filter_center.fulfilled( (toArray(icyl.Location()),) ): ok=False
	  
	if ok:
	  faces.add(fi)
	  
    return faces
    
  def faceClasses(self):
    classes={}
    for fi in range(1, self.fmap.Extent()+1):
      face=TopoDS.face(self.fmap(fi))
      adapt=GeomAdaptor_Surface(BRep_Tool.Surface(face))
      classes[fi]=adapt.GetType()
    return classes


  def findPlanarFaces(self, filter_direction=None, filter_area=None, deviation=SMALL):
    faces=set()
    for fi in range(1, self.fmap.Extent()+1):
      face=TopoDS.face(self.fmap(fi))
      adapt=GeomAdaptor_Surface(BRep_Tool.Surface(face))
      if adapt.GetType()==GeomAbs.GeomAbs_Plane:
	prop=BRepGProp_Face(face)
	u1,u2,v1,v2=prop.Bounds()
	u = (u1+u2)/2;
	v = (v1+v2)/2;
	vec=gp_Vec()
	pnt=gp_Pnt()
	prop.Normal(u,v,pnt,vec)
	
	ok=True
	if not filter_direction is None:
	  if not filter_direction.fulfilled( (toArray(vec), toArray(pnt)) ): ok=False

	if not filter_area is None:
	  ar=faceArea(face)
	  #print "%d: plane"%fi, ", area=", ar
	  if not abs(ar-filter_area) < deviation: ok=False
	    
	if ok:
	  faces.add(fi)
	  
    return faces

  def findBSplineFaces(self, filter_area=None, deviation=SMALL):
    faces=set()
    for fi in range(1, self.fmap.Extent()+1):
      face=TopoDS.face(self.fmap(fi))
      adapt=GeomAdaptor_Surface(BRep_Tool.Surface(face))
      if adapt.GetType()==GeomAbs.GeomAbs_BSplineSurface:
	#prop=BRepGProp_Face(face)
	#u1,u2,v1,v2=prop.Bounds()
	#u = (u1+u2)/2;
	#v = (v1+v2)/2;
	#vec=gp_Vec()
	#pnt=gp_Pnt()
	#prop.Normal(u,v,pnt,vec)
	
	ok=True
	#if not filter_direction is None:
	#  if not filter_direction.fulfilled( (toArray(vec), toArray(pnt)) ): ok=False

	if not filter_area is None:
	  ar=faceArea(face)
	  #print "%d: plane"%fi, ", area=", ar
	  if not abs(ar-filter_area) < deviation: ok=False
	    
	if ok:
	  faces.add(fi)
	  
    return faces

  def findFacesConnecting(self, faces):
    rfaces=set()
    org_edges=[self.edgesOfFace(TopoDS.face(self.fmap(fi))) for fi in faces]
    for fi in range(1, self.fmap.Extent()+1):
      face=TopoDS.face(self.fmap(fi))
      edges=self.edgesOfFace(face)
      num_match=0
      for j in org_edges:
	if len(j & edges)>0: num_match+=1
      if num_match>=2: rfaces.add(fi)
    return rfaces
      

  def edgesOfFaces(self, faces):
    edges=set()
    for fi in faces:
      edges.update(self.edgesOfFace(TopoDS.face(self.fmap(fi))))
    return edges     

  def verticesOfEdges(self, edges):
    vertices=set()
    for ei in edges:
      vertices.update(self.verticesOfEdge(TopoDS.edge(self.emap(ei))))
    return vertices     

  def verticesOfFaces(self, faces):
    vertices=set()
    for fi in faces:
      vertices.update(self.verticesOfFace(TopoDS.face(self.fmap(fi))))
    return vertices     

  def facesFaces(self, faces):
    rf=set()
    for fi in faces:
      rf.update(self.faceFaces(fi))
    return rf

  def faceCoG(self, fi):
    return faceCoG(TopoDS.face(self.fmap(fi)))
    
  def facesCoG(self, faces):
    return {fi: self.faceCoG(fi) for fi in faces}

  def cylFaceRadius(self, fi):
    return cylFaceRadius(TopoDS.face(self.fmap(fi)))
    
  def cylFacesRadius(self, faces):
    return {fi: self.cylFaceRadius(fi) for fi in faces}

  def nearestFace(self, fi, others=None):
    return set([self.nearestFaces(fi, others)[0][1]])
    
  def nearestFaces(self, face, others=None):
    p0=faceCoG(face)
    if others is None:
      others=set(range(1, self.fmap.Extent()+1))
    p_others=self.facesCoG(others)
    #print p0, p_others
    dist=[(npla.norm(cog-p0), f) for f,cog in p_others.items()]
    #print dist
    return sorted(dist)
    

def classify(d, comp, deviation=SMALL):
  fis=d.keys()
  vals=[d[k][comp] for k in fis]
  minv=float(min(vals))
  maxv=float(max(vals))
  nbins=(maxv-minv)/float(deviation)
  bins=np.linspace(minv, maxv, nbins)
  #print minv, maxv, len(bins), bins
  bin_idxs=np.digitize(vals, bins)
  #print bin_idxs
  rlist={}
  for i,fi in enumerate(fis):
    rlist.setdefault(bins[bin_idxs[i]-1], set()).add(fi)
  return rlist
    
    
def sortedList(d):
  return sorted(d.items(), key=lambda x: x[1])
  
def sortedListBy(d, comp):
  return sorted(d.items(), key=lambda x: x[1][comp])
  
def single(s):
  if len(s)!=1: raise Exception("Length of set should be =1, got "+str(s))
  return iter(s).next()
  

  
class GmshCase(object):
  def __init__(self, part, Lmax=500., Lmin=0.1, volname=None):
    self.part=part
    self.volname=self.part.name if volname is None else volname
    self.inputfilename=self.part.name+".geo"
    self.minLength=Lmin
    self.maxLength=Lmax
    self.secondOrderLinear=0
    self.options=[]
    
  def doMeshing(self, force=False):
    if os.path.exists(os.path.splitext(self.inputfilename)[0]+".med") and not force: return
    self.writeInput()
    if (subprocess.call(['gmsh', '-3', '-optimize_netgen', self.inputfilename])!=0):
     raise Exception("gmsh failed!")
    
  def setEdgeLen(self, en, L):
    self.options.append("Characteristic Length{%s}=%g;"%(
      ",".join(["%d"%pi for pi in self.part.verticesOfEdges(self.part.namedEdges[en])]),
      L
      ))
    
  def setFaceEdgeLen(self, fn, L):
    self.options.append("Characteristic Length{%s}=%g;"%(
      ",".join(["%d"%pi for pi in self.part.verticesOfFaces(self.part.namedFaces[fn])]),
      L
      ))
    
  def writeInput(self):
    f=open(self.inputfilename, "w")
    f.write("""\
Geometry.AutoCoherence = 0;
Geometry.OCCSewFaces = 0;
Geometry.Tolerance = 1e-10;
//Mesh.LcIntegrationPrecision = 1e-12;

Merge "%s";

%s
%s
%s

Physical Volume("%s") = {1};

//Mesh.Algorithm = 1; /* 1=MeshAdapt, 2=Automatic, 5=Delaunay, 6=Frontal, 7=bamg, 8=delquad */
//Mesh.Algorithm3D = 4; /* 1=Delaunay, 4=Frontal, 5=Frontal Delaunay, 6=Frontal Hex, 7=MMG3D, 9=R-tree */
Mesh.CharacteristicLengthMin = %g;
Mesh.CharacteristicLengthMax = %g;
Mesh.ElementOrder=2;
Mesh.SecondOrderLinear=%d;
Mesh.Smoothing = 10;
Mesh.SmoothNormals = 1;
Mesh.Explode = 1;
Mesh.Format=33; /* 1=msh, 2=unv, 10=automatic, 19=vrml, 27=stl, 30=mesh, 31=bdf, 32=cgns, 33=med, 40=ply2 */

"""%(
self.part.filename,
"\n".join([str(o) for o in self.options]),
"\n".join(["Physical Line(\"%s\") = {%s};"%(en, ",".join([str(i) for i in es])) for en, es in self.part.namedEdges.items()]),
"\n".join(["Physical Surface(\"%s\") = {%s};"%(fn, ",".join([str(i) for i in fs])) for fn, fs in self.part.namedFaces.items()]),
self.volname,
self.minLength,
self.maxLength,
self.secondOrderLinear
))
    f.close()
