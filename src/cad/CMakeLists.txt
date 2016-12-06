project(insightcad)

find_package(OpenCasCade REQUIRED)
find_package(OpenGL)
find_package(DXFlib)
find_package(GEOM)


set(insightcad_SOURCES 
  astbase.cpp
  cadpostprocaction.cpp
  dxfwriter.cpp 
  parser.cpp 
  parser_selectionexpressions.cpp
  parser_datumexpressions.cpp
  parser_featureexpressions.cpp
  parser_postprocexpressions.cpp
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
)

set(INCLUDE_DIRS "")
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
add_subdirectory(quantitycomputers)
add_subdirectory(featurefilters)
add_subdirectory(cadfeatures)
add_subdirectory(cadparameters)
add_subdirectory(cadpostprocactions)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${toolkit_SOURCE_DIR})

add_library(insightcad SHARED ${insightcad_SOURCES})
target_link_libraries(insightcad toolkit ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES} ${DXFLIB_LIBRARY} ${GEOM_LIBRARIES} dlib)

## PROFILING
set_target_properties(insightcad PROPERTIES COMPILE_FLAGS -g0)

target_include_directories(insightcad
  PUBLIC ${toolkit_SOURCE_DIR}
  PUBLIC ${Boost_INCLUDE_DIRS}
  PUBLIC ${OCC_INCLUDE_DIR}
  PUBLIC ${DXFLIB_INCLUDE_DIR}
  PUBLIC ${GEOM_INCLUDE_DIRS}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${INCLUDE_DIRS}
  )
install(TARGETS insightcad LIBRARY DESTINATION lib)

add_pybindings(cad "cad.i" insightcad)

add_subdirectory(iscad)
