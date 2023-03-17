project(insightcad)

find_package(OCC REQUIRED)
find_package(DXFlib)

FIND_PACKAGE(MED)

if (MED_FOUND)
    ADD_DEFINITIONS("-DHAVE_MED")
endif()

set(insightcad_SOURCES 
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
  constrainedsketchgeometry.h constrainedsketchgeometry.cpp
  sketch.h sketch.cpp
  geotest.cpp 
  cadparameter.cpp
  cadfeature.cpp 
  featurecache.cpp
  cadmodel.cpp
  meshing.h
  meshing/gmshcase.h meshing/gmshcase.cpp
  meshing/surfacegmshcase.h meshing/surfacegmshcase.cpp
  meshing/sheetextrusiongmshcase.h meshing/sheetextrusiongmshcase.cpp
  featurefilter.cpp 
  feature.cpp
  occtools.cpp
  caddocitem.cpp
  cadtypes.cpp
  subshapenumbering.cpp subshapenumbering.h
  polytriangulationelementiterator.h polytriangulationelementiterator.cpp
  polytriangulationnodeiterator.h polytriangulationnodeiterator.cpp
  cademesh.h cademesh.cpp
  ivtkoccshape.h ivtkoccshape.cpp
  cadgeometryparameter.h cadgeometryparameter.cpp
)

if (OCC_VERSION_MAJOR LESS 7)
 list(APPEND insightcad_SOURCES
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

set(INCLUDE_DIRS "")
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
add_subdirectory(quantitycomputers)
add_subdirectory(featurefilters)
add_subdirectory(cadfeatures)
add_subdirectory(cadparameters)
add_subdirectory(cadpostprocactions)
add_subdirectory(docitems)
add_subdirectory(test)

add_library(insightcad SHARED ${insightcad_SOURCES})
target_link_libraries(insightcad 
 toolkit 
 ${OCC_LIBRARIES} # TKXSDRAW # don't include: leads to segfault on program exit!
 ${DXFLIB_LIBRARY} 
 ${MED_LIBRARIES}
 )
include(GenerateExportHeader)
generate_export_header(insightcad)

## PROFILING
# set_target_properties(insightcad PROPERTIES COMPILE_FLAGS -g0)

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

add_subdirectory(insightcad_gui)
add_subdirectory(iscad)
