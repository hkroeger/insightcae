add_sources(insightcad_SOURCES 
 derivedfeature.cpp derivedfeature.h
 singleedgefeature.cpp singleedgefeature.h

 singlevertex.h singlevertex.cpp
 arc.cpp arc.h
 airfoil.cpp airfoil.h
 bar.cpp bar.h
 booleanintersection.cpp booleanintersection.h
 booleansubtract.cpp booleansubtract.h
 booleanunion.cpp booleanunion.h
 boundedflatface.cpp boundedflatface.h
 box.cpp box.h
 chamfer.cpp chamfer.h
 circle.cpp circle.h
 clipwire.cpp clipwire.h
 circularpattern.cpp circularpattern.h
 compound.cpp compound.h
 cutaway.cpp cutaway.h
 cutup.cpp cutup.h
 cylinder.cpp cylinder.h
 cone.cpp cone.h
 torus.cpp torus.h
 extrusion.cpp extrusion.h
 ellipse.cpp ellipse.h
 fillet.cpp fillet.h
 fillingface.cpp fillingface.h
 fixshape.cpp fixshape.h
 faceisocurve.cpp faceisocurve.h
 gluefaces.cpp gluefaces.h
 importsolidmodel.cpp importsolidmodel.h
 linearpattern.cpp linearpattern.h
 line.cpp line.h
 mirror.cpp mirror.h
 pipe.cpp pipe.h
 place.cpp place.h
 projected.cpp projected.h
 projectedoutline.cpp projectedoutline.h
 quad.cpp quad.h
 regpoly.cpp regpoly.h
 polygon.cpp polygon.h
 revolution.cpp revolution.h
 ring.cpp ring.h
 rotatedhelicalsweep.cpp rotatedhelicalsweep.h
 shoulder.cpp shoulder.h
 sphere.cpp sphere.h
 splinecurve.cpp splinecurve.h
 splinesurface.cpp splinesurface.h
 split.cpp split.h
 spring.cpp spring.h
 sinewave.cpp sinewave.h
 helix.cpp helix.h
 spiral.cpp spiral.h
 stitchedcompound.cpp stitchedcompound.h
 stitchedshell.cpp stitchedshell.h
 stitchedsolid.cpp stitchedsolid.h
 stringer.cpp stringer.h

 stl.cpp stl.h

 sweep.cpp sweep.h
 sheet.cpp sheet.h
 transform.cpp transform.h
 tri.cpp tri.h
 tongue.cpp tongue.h
 wire.cpp wire.h
 wirefillet.cpp wirefillet.h
 modelfeature.cpp modelfeature.h
 maxsurfacecurvature.cpp maxsurfacecurvature.h
 subfeature.cpp subfeature.h
 nacafourdigit.cpp nacafourdigit.h
 coil.cpp  coil.h
 freecadmodel.cpp freecadmodel.h
 pyramid.cpp pyramid.h
 closedpolyline.cpp closedpolyline.h
 loop.cpp loop.h
 gear.cpp gear.h
 curvepattern.cpp curvepattern.h
 refplace.cpp refplace.h
 exploded.cpp exploded.h
 partition.cpp partition.h
 offset.cpp offset.h
 
  GEOMAlgo_Splitter.cxx
  
  GEOMAlgo_Gluer2.cxx
  GEOMAlgo_Gluer2_1.cxx
  GEOMAlgo_Gluer2_2.cxx
  GEOMAlgo_Gluer2_3.cxx
  GEOMAlgo_GluerAlgo.cxx
  GEOMAlgo_BuilderShape.cxx
  GEOMAlgo_Algo.cxx
  GEOMAlgo_GlueDetector.cxx
  GEOMAlgo_AlgoTools.cxx
  GEOMAlgo_AlgoTools_1.cxx
  GEOMAlgo_BndSphereTree.cxx
  GEOMAlgo_BndSphere.cxx
  GEOMAlgo_PassKey.cxx
  GEOMAlgo_PassKeyShape.cxx
  GEOMAlgo_PassKeyMapHasher.cxx
  GEOMAlgo_PassKeyShapeMapHasher.cxx
  GEOMAlgo_CoupleOfShapes.cxx
)

add_include_dir(INCLUDE_DIRS)
