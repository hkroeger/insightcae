#!/usr/bin/python

from OCC.BRepAlgoAPI import *
from OCC.BRepPrimAPI import *
from OCC.BRepBuilderAPI import *
from Insight.Aster.geometry import *
import pprint
    
axis=np.array([0,0,1.])
e_up=np.array([0,1,0])
e_lat=np.array([1,0.,0])
#boltlength=27.0

  
p=[None]*2

loc_heads={}
loc_nuts={}

part=p[0]=SolidBody("base.brep")
pprint.pprint(part.faceClasses())
part.nameFaces("1n0", part.findCylindricalFaces(filter_inside=True))
loc_nuts["s0"]=part.faceCoG(single(part.namedFaces["1n0"]))
part.nameFaces("1lat", part.findPlanarFaces(filter_direction=vecAligned(e_lat, sense=False)))
part.nameFaces("1axc", part.findPlanarFaces(filter_direction=vecAligned(axis)))

part=p[1]=SolidBody("part.brep")
pprint.pprint(part.faceClasses())
ff=part.findPlanarFaces(filter_area=math.pi*(12**2-8**2)*0.25, deviation=1)
part.nameFaces("2h0", ff)
loc_heads["s0"]=part.faceCoG(single(part.namedFaces["2h0"]))
part.nameFaces("2lat", part.findPlanarFaces(filter_direction=vecAligned(e_lat, sense=False)))
part.nameFaces("2axc", part.findPlanarFaces(filter_direction=vecAligned(-axis)))

  
bolts=GmshBoltMesh()
for n in loc_heads:
  bolts.addBolt(loc_heads[n], loc_nuts[n], n)
bolts.doMeshing()

for i in range(0, len(p)):
  print p[i]

msh=GmshCase(p[0], 5., volname='v_1')
msh.setFaceEdgeLen("1n0", 2.)
msh.elementOrder=1
msh.doMeshing()

msh=GmshCase(p[1], 4., volname='v_2')
msh.elementOrder=1
msh.setFaceEdgeLen("2h0", 2.)
msh.doMeshing()

