project(insightcad)

find_package(OCC REQUIRED)
find_package(DXFlib)
find_package(MED)


if (MED_FOUND)
    ADD_DEFINITIONS("-DHAVE_MED")
endif()


if (OCC_VERSION_MAJOR LESS 7)
 list(APPEND insightcad_ADDITIONAL_SOURCES
  compat/BRepBndLib.cxx compat/BRepBndLib_1.cxx compat/BRepBndLib.hxx
  compat/Bnd_OBB.cxx compat/Bnd_OBB.hxx compat/Bnd_Tools.hxx
  compat/Bnd_Range.hxx compat/Bnd_Range.cxx
  compat/BndLib_Add2dCurve.cxx  compat/BndLib_Add2dCurve.hxx
  compat/BndLib_Add3dCurve.cxx  compat/BndLib_Add3dCurve.hxx
  compat/BndLib_AddSurface.cxx  compat/BndLib_AddSurface.hxx
  compat/BVH_BoxSet.hxx
  compat/BVH_Traverse.hxx
 )
endif()



add_library(insightcad SHARED
  treeclonemap.h treeclonemap.cpp
  dependencysource.h dependencysource.cpp
  dependencyreplacement.h dependencyreplacement.cpp

  cadexception.h cadexception.cpp
  translations.cpp
  astbase.cpp
  cadpostprocaction.cpp
  dxfwriter.cpp
  dxfreader.cpp
  parser.cpp
  parser_selectionexpressions.cpp
  parser_datumexpressions.cpp
  parser_featureexpressions.cpp
  parser_postprocexpressions.cpp
  parser_docexpressions.cpp
  parser_scalarexpressions.cpp
  parser_vectorexpressions.cpp
  datum.cpp

  sketch.h sketch.cpp

  constrainedsketch.h constrainedsketch.cpp
  constrainedsketchentity.h constrainedsketchentity.cpp
  constrainedsketchentities/sketchpoint.h constrainedsketchentities/sketchpoint.cpp
  constrainedsketchentities/singlesymbolconstraint.h constrainedsketchentities/singlesymbolconstraint.cpp
  constrainedsketchentities/fixedpointconstraint.h constrainedsketchentities/fixedpointconstraint.cpp
  constrainedsketchentities/horizontalconstraint.h constrainedsketchentities/horizontalconstraint.cpp
  constrainedsketchentities/verticalconstraint.h constrainedsketchentities/verticalconstraint.cpp
  constrainedsketchentities/pointoncurveconstraint.h constrainedsketchentities/pointoncurveconstraint.cpp
  constrainedsketchentities/constraintwithdimensionlines.h constrainedsketchentities/constraintwithdimensionlines.cpp
  constrainedsketchentities/angleconstraint.h constrainedsketchentities/angleconstraint.cpp
  constrainedsketchentities/distanceconstraint.h constrainedsketchentities/distanceconstraint.cpp
  constrainedsketchentities/externalreference.h constrainedsketchentities/externalreference.cpp
  constrainedsketchentities/tangentconstraint.h constrainedsketchentities/tangentconstraint.cpp
  constrainedsketchgrammar.h constrainedsketchgrammar.cpp

  geotest.cpp

  parameterlisthash.h parameterlisthash.cpp
  cadparameter.cpp
  cadfeature.cpp
  featurecache.cpp
  cadmodel.cpp
  meshing.h
  meshing/gmshcase.h meshing/gmshcase.cpp
  meshing/surfacegmshcase.h meshing/surfacegmshcase.cpp
  meshing/sheetextrusiongmshcase.h meshing/sheetextrusiongmshcase.cpp
  featurefilter.cpp
  featureset.h featureset.cpp
  occtools.cpp
  caddocitem.cpp
  cadtypes.cpp
  subshapenumbering.cpp subshapenumbering.h
  polytriangulationelementiterator.h polytriangulationelementiterator.cpp
  polytriangulationnodeiterator.h polytriangulationnodeiterator.cpp
  cademesh.h cademesh.cpp
  ivtkoccshape.h ivtkoccshape.cpp

  cadparameters/constantscalar.cpp
  cadparameters/constantscalar.h
  cadparameters/constantvector.cpp
  cadparameters/constantvector.h
  cadparameters/lookuptablescalar.cpp
  cadparameters/lookuptablescalar.h
  cadparameters/loopscalar.cpp
  cadparameters/loopscalar.h
  cadparameters/projectedpoint.cpp
  cadparameters/projectedpoint.h
  cadparameters/scalarfeatureprop.cpp
  cadparameters/scalarfeatureprop.h
  cadparameters/scalarops.cpp
  cadparameters/scalarops.h
  cadparameters/vectorfeatureprop.cpp
  cadparameters/vectorfeatureprop.h
  cadparameters/vectorops.cpp
  cadparameters/vectorops.h

  quantitycomputers/distance.cpp
  quantitycomputers/distance.h
  quantitycomputers/vertexlocation.cpp
  quantitycomputers/vertexlocation.h
  quantitycomputers/edgecog.cpp
  quantitycomputers/edgecog.h
  quantitycomputers/edgelen.cpp
  quantitycomputers/edgelen.h
  quantitycomputers/edgemidtangent.cpp
  quantitycomputers/edgemidtangent.h

  quantitycomputers/constantquantity.h
  quantitycomputers/cylaxis.cpp
  quantitycomputers/cylaxis.h
  quantitycomputers/cylradius.cpp
  quantitycomputers/cylradius.h
  quantitycomputers/edgeend.cpp
  quantitycomputers/edgeend.h
  quantitycomputers/edgeradiallen.cpp
  quantitycomputers/edgeradiallen.h
  quantitycomputers/edgestart.cpp
  quantitycomputers/edgestart.h
  quantitycomputers/facearea.cpp
  quantitycomputers/facearea.h
  quantitycomputers/facecog.cpp
  quantitycomputers/facecog.h
  quantitycomputers/facenormalvector.cpp
  quantitycomputers/facenormalvector.h
  quantitycomputers/quantityfunctions.h
  quantitycomputers/solidcog.cpp
  quantitycomputers/solidcog.h
  quantitycomputers/solidvolume.cpp
  quantitycomputers/solidvolume.h

  featurefilters/approximatelyequal.h
  featurefilters/booleanfilters.cpp
  featurefilters/booleanfilters.h
  featurefilters/boundaryedge.cpp
  featurefilters/boundaryedge.h
  featurefilters/boundaryofface.cpp
  featurefilters/boundaryofface.h
  featurefilters/coincident.cpp
  featurefilters/coincident.h
  featurefilters/coincidentprojectededge.cpp
  featurefilters/coincidentprojectededge.h
  featurefilters/commonfacearea.cpp
  featurefilters/commonfacearea.h
  featurefilters/connected.cpp
  featurefilters/connected.h
  featurefilters/cylfaceorientation.cpp
  featurefilters/cylfaceorientation.h
  featurefilters/edgeconnectingvertices.cpp
  featurefilters/edgeconnectingvertices.h
  featurefilters/edgetopology.cpp
  featurefilters/edgetopology.h
  featurefilters/everything.cpp
  featurefilters/everything.h
  featurefilters/extremal.cpp
  featurefilters/extremal.h
  featurefilters/faceadjacenttoedges.cpp
  featurefilters/faceadjacenttoedges.h
  featurefilters/faceadjacenttofaces.cpp
  featurefilters/faceadjacenttofaces.h
  featurefilters/facetopology.cpp
  featurefilters/facetopology.h
  featurefilters/identical.cpp
  featurefilters/identical.h
  featurefilters/in.cpp
  featurefilters/in.h
  featurefilters/ispartofface.cpp
  featurefilters/ispartofface.h
  featurefilters/ispartofsolid.cpp
  featurefilters/ispartofsolid.h
  featurefilters/maximal.cpp
  featurefilters/maximal.h
  featurefilters/minimal.cpp
  featurefilters/minimal.h
  featurefilters/relationfilters.h
  featurefilters/same.cpp
  featurefilters/same.h
  featurefilters/secant.cpp
  featurefilters/secant.h

  cadfeatures/airfoil.cpp
  cadfeatures/airfoil.h
  cadfeatures/alignwithboundingbox.cpp
  cadfeatures/alignwithboundingbox.h
  cadfeatures/arc.cpp
  cadfeatures/arc.h
  cadfeatures/bar.cpp
  cadfeatures/bar.h
  cadfeatures/booleanintersection.cpp
  cadfeatures/booleanintersection.h
  cadfeatures/booleansubtract.cpp
  cadfeatures/booleansubtract.h
  cadfeatures/booleanunion.cpp
  cadfeatures/booleanunion.h
  cadfeatures/boundedflatface.cpp
  cadfeatures/boundedflatface.h
  cadfeatures/box.cpp
  cadfeatures/box.h
  cadfeatures/chamfer.cpp
  cadfeatures/chamfer.h
  cadfeatures/circle.cpp
  cadfeatures/circle.h
  cadfeatures/circularpattern.cpp
  cadfeatures/circularpattern.h
  cadfeatures/clipwire.cpp
  cadfeatures/clipwire.h
  cadfeatures/closedpolyline.cpp
  cadfeatures/closedpolyline.h
  cadfeatures/coil.cpp
  cadfeatures/coil.h
  cadfeatures/compound.cpp
  cadfeatures/compound.h
  cadfeatures/cone.cpp
  cadfeatures/cone.h
  cadfeatures/curvepattern.cpp
  cadfeatures/curvepattern.h
  cadfeatures/cutaway.cpp
  cadfeatures/cutaway.h
  cadfeatures/cutup.cpp
  cadfeatures/cutup.h
  cadfeatures/cylinder.cpp
  cadfeatures/cylinder.h
  cadfeatures/derivedfeature.cpp
  cadfeatures/derivedfeature.h
  cadfeatures/ellipse.cpp
  cadfeatures/ellipse.h
  cadfeatures/exploded.cpp
  cadfeatures/exploded.h
  cadfeatures/extrusion.cpp
  cadfeatures/extrusion.h
  cadfeatures/faceisocurve.cpp
  cadfeatures/faceisocurve.h
  cadfeatures/fillet.cpp
  cadfeatures/fillet.h
  cadfeatures/fillingface.cpp
  cadfeatures/fillingface.h
  cadfeatures/fixshape.cpp
  cadfeatures/fixshape.h
  cadfeatures/forloop.cpp
  cadfeatures/forloop.h
  cadfeatures/freecadmodel.cpp
  cadfeatures/freecadmodel.h
  cadfeatures/gear.cpp
  cadfeatures/gear.h
  cadfeatures/gluefaces.cpp
  cadfeatures/gluefaces.h
  cadfeatures/helix.cpp
  cadfeatures/helix.h
  cadfeatures/importsolidmodel.cpp
  cadfeatures/importsolidmodel.h
  cadfeatures/inhomscale.cpp
  cadfeatures/inhomscale.h
  cadfeatures/linearpattern.cpp
  cadfeatures/linearpattern.h
  cadfeatures/line.cpp
  cadfeatures/line.h
  cadfeatures/maxsurfacecurvature.cpp
  cadfeatures/maxsurfacecurvature.h
  cadfeatures/mirror.cpp
  cadfeatures/mirror.h
  cadfeatures/modelfeature.cpp
  cadfeatures/modelfeature.h
  cadfeatures/nacafourdigit.cpp
  cadfeatures/nacafourdigit.h
  cadfeatures/offset.cpp
  cadfeatures/offset.h
  cadfeatures/partition.cpp
  cadfeatures/partition.h
  cadfeatures/pipe.cpp
  cadfeatures/pipe.h
  cadfeatures/place.cpp
  cadfeatures/place.h
  cadfeatures/polygon.cpp
  cadfeatures/polygon.h
  cadfeatures/projected.cpp
  cadfeatures/projected.h
  cadfeatures/projectedoutline.cpp
  cadfeatures/projectedoutline.h
  cadfeatures/pyramid.cpp
  cadfeatures/pyramid.h
  cadfeatures/quad.cpp
  cadfeatures/quad.h
  cadfeatures/refplace.cpp
  cadfeatures/refplace.h
  cadfeatures/regpoly.cpp
  cadfeatures/regpoly.h
  cadfeatures/revolution.cpp
  cadfeatures/revolution.h
  cadfeatures/ring.cpp
  cadfeatures/ring.h
  cadfeatures/rotatedhelicalsweep.cpp
  cadfeatures/rotatedhelicalsweep.h
  cadfeatures/sheet.cpp
  cadfeatures/sheet.h
  cadfeatures/shoulder.cpp
  cadfeatures/shoulder.h
  cadfeatures/sinewave.cpp
  cadfeatures/sinewave.h
  cadfeatures/singleedgefeature.cpp
  cadfeatures/singleedgefeature.h
  cadfeatures/singlevertex.cpp
  cadfeatures/singlevertex.h
  cadfeatures/sphere.cpp
  cadfeatures/sphere.h
  cadfeatures/spiral.cpp
  cadfeatures/spiral.h
  cadfeatures/splinecurve.cpp
  cadfeatures/splinecurve.h
  cadfeatures/splinesurface.cpp
  cadfeatures/splinesurface.h
  cadfeatures/split.cpp
  cadfeatures/split.h
  cadfeatures/splitsurface.cpp
  cadfeatures/splitsurface.h
  cadfeatures/spring.cpp
  cadfeatures/spring.h
  cadfeatures/stitchedcompound.cpp
  cadfeatures/stitchedcompound.h
  cadfeatures/stitchedshell.cpp
  cadfeatures/stitchedshell.h
  cadfeatures/stitchedsolid.cpp
  cadfeatures/stitchedsolid.h
  cadfeatures/sailcut3d.cpp
  cadfeatures/sailcut3d.h
  cadfeatures/stl.cpp
  cadfeatures/stl.h
  cadfeatures/stringer.cpp
  cadfeatures/stringer.h
  cadfeatures/subfeature.cpp
  cadfeatures/subfeature.h
  cadfeatures/sweep.cpp
  cadfeatures/sweep.h
  cadfeatures/tongue.cpp
  cadfeatures/tongue.h
  cadfeatures/torus.cpp
  cadfeatures/torus.h
  cadfeatures/transform.cpp
  cadfeatures/transform.h
  cadfeatures/tri.cpp
  cadfeatures/tri.h
  cadfeatures/wire.cpp
  cadfeatures/wirefillet.cpp
  cadfeatures/wirefillet.h
  cadfeatures/wire.h

  cadfeatures/GEOMAlgo_Algo.cxx
  cadfeatures/GEOMAlgo_Algo.hxx
  cadfeatures/GEOMAlgo_AlgoTools_1.cxx
  cadfeatures/GEOMAlgo_AlgoTools.cxx
  cadfeatures/GEOMAlgo_AlgoTools.hxx
  cadfeatures/GEOMAlgo_BndSphere.cxx
  cadfeatures/GEOMAlgo_BndSphere.hxx
  cadfeatures/GEOMAlgo_BndSphere.lxx
  cadfeatures/GEOMAlgo_BndSphereTree.cxx
  cadfeatures/GEOMAlgo_BndSphereTree.hxx
  cadfeatures/GEOMAlgo_BuilderShape.cxx
  cadfeatures/GEOMAlgo_BuilderShape.hxx
  cadfeatures/GEOMAlgo_CoupleOfShapes.cxx
  cadfeatures/GEOMAlgo_CoupleOfShapes.hxx
  cadfeatures/GEOMAlgo_GlueDetector.cxx
  cadfeatures/GEOMAlgo_GlueDetector.hxx
  cadfeatures/GEOMAlgo_Gluer2_1.cxx
  cadfeatures/GEOMAlgo_Gluer2_2.cxx
  cadfeatures/GEOMAlgo_Gluer2_3.cxx
  cadfeatures/GEOMAlgo_Gluer2.cxx
  cadfeatures/GEOMAlgo_Gluer2.hxx
  cadfeatures/GEOMAlgo_GluerAlgo.cxx
  cadfeatures/GEOMAlgo_GluerAlgo.hxx
  cadfeatures/GEOMAlgo_IndexedDataMapOfIntegerShape.hxx
  cadfeatures/GEOMAlgo_IndexedDataMapOfPassKeyShapeListOfShape.hxx
  cadfeatures/GEOMAlgo_IndexedDataMapOfShapeBndSphere.hxx
  cadfeatures/GEOMAlgo_IndexedDataMapOfShapeIndexedMapOfShape.hxx
  cadfeatures/GEOMAlgo_ListIteratorOfListOfCoupleOfShapes.hxx
  cadfeatures/GEOMAlgo_ListOfCoupleOfShapes.hxx
  cadfeatures/GEOMAlgo_PassKey.cxx
  cadfeatures/GEOMAlgo_PassKey.hxx
  cadfeatures/GEOMAlgo_PassKeyMapHasher.cxx
  cadfeatures/GEOMAlgo_PassKeyMapHasher.hxx
  cadfeatures/GEOMAlgo_PassKeyShape.cxx
  cadfeatures/GEOMAlgo_PassKeyShape.hxx
  cadfeatures/GEOMAlgo_PassKeyShapeMapHasher.cxx
  cadfeatures/GEOMAlgo_PassKeyShapeMapHasher.hxx
  cadfeatures/GEOMAlgo_Splitter.cxx
  cadfeatures/GEOMAlgo_Splitter.hxx

  cadpostprocactions/angle.cpp
  cadpostprocactions/angle.h
  cadpostprocactions/bom.cpp
  cadpostprocactions/bom.h
  cadpostprocactions/drawingexport.cpp
  cadpostprocactions/drawingexport.h
  cadpostprocactions/export.cpp
  cadpostprocactions/export.h
  cadpostprocactions/framemesh.cpp
  cadpostprocactions/framemesh.h
  cadpostprocactions/hydrostatics.cpp
  cadpostprocactions/hydrostatics.h
  cadpostprocactions/mesh.cpp
  cadpostprocactions/mesh.h
  cadpostprocactions/pointdistance.cpp
  cadpostprocactions/pointdistance.h
  cadpostprocactions/solidproperties.cpp
  cadpostprocactions/solidproperties.h

  docitems/diameter.cpp
  docitems/diameter.h

  ${insightcad_ADDITIONAL_SOURCES}
  parser_tools.h
  featurefilter_vertex.cpp
  featurefilter_edge.cpp
  featurefilter_face.cpp
  featurefilter_solid.cpp
)
target_link_libraries(insightcad
 toolkit
 ${OCC_LIBRARIES} # TKXSDRAW # don't include: leads to segfault on program exit!
 ${DXFLIB_LIBRARY}
 ${MED_LIBRARIES}
 )


target_setup_gettext(insightcad insightcad)

## disabled: propduces issue due to "using namespace" statements
# set_target_properties(insightcad PROPERTIES
#    UNITY_BUILD ON
#    UNITY_BUILD_BATCH_SIZE 16
# )

include(GenerateExportHeader)
generate_export_header(insightcad)



target_include_directories(insightcad
  PUBLIC ${OCC_INCLUDE_DIR}
  PUBLIC ${DXFLIB_INCLUDE_DIR}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  PUBLIC ${MED_INCLUDE_DIRS}
  )


if (INSIGHT_DEVELOPMENT_FILES)
 install(
    TARGETS insightcad EXPORT insightcad_targets
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
 export(EXPORT insightcad_targets
  FILE "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAECADTarget.cmake"
  NAMESPACE insight::
 )
 install(EXPORT insightcad_targets
  FILE InsightCAECADTarget.cmake
  NAMESPACE insight::
  DESTINATION ${INSIGHT_ConfigPackageLocation}
 )
else()
 install(
    TARGETS insightcad
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
endif()

install(TARGETS insightcad LIBRARY DESTINATION lib)

add_pybindings(cad "cad.i" insightcad)

add_subdirectory(iscad)
