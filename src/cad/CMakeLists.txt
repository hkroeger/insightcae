project(insightcad)

find_package(OCC REQUIRED)
find_package(OpenGL)
find_package(DXFlib)
#find_package(GEOM)
#find_package(dlib)


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
  sketch.cpp 
  geotest.cpp 
  cadparameter.cpp
  cadfeature.cpp 
  cadmodel.cpp
  meshing.cpp 
  featurefilter.cpp 
  feature.cpp
  occtools.cpp
  caddocitem.cpp
  cadtypes.cpp
)

set(INCLUDE_DIRS "")
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
add_subdirectory(quantitycomputers)
add_subdirectory(featurefilters)
add_subdirectory(cadfeatures)
add_subdirectory(cadparameters)
add_subdirectory(cadpostprocactions)
add_subdirectory(docitems)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${toolkit_SOURCE_DIR})

add_library(insightcad SHARED ${insightcad_SOURCES})
target_link_libraries(insightcad 
 toolkit 
 ${OCC_LIBRARIES} # TKXSDRAW # don't include: leads to segfault on program exit!
 ${OCC_OCAF_LIBRARIES} 
 ${DXFLIB_LIBRARY} 
 #${GEOM_LIBRARIES}
 #${DLIB_LIBRARIES}
 )

## PROFILING
# set_target_properties(insightcad PROPERTIES COMPILE_FLAGS -g0)

target_include_directories(insightcad
  PUBLIC ${Boost_INCLUDE_DIRS}
  PUBLIC ${OCC_INCLUDE_DIR}
  PUBLIC ${DXFLIB_INCLUDE_DIR}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  )

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

install(TARGETS insightcad LIBRARY DESTINATION lib)

add_pybindings(cad "cad.i" insightcad)

add_subdirectory(insightcad_gui)
add_subdirectory(iscad)
