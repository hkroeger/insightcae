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

# include_directories(${toolkit_SOURCE_DIR})
# include_directories(${Boost_INCLUDE_DIRS})
# include_directories(${OCC_INCLUDE_DIR})
# include_directories(${DXFLIB_INCLUDE_DIR})
# include_directories(${GEOM_INCLUDE_DIRS})
include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${toolkit_SOURCE_DIR})

#link_directories(${OCC_LIBRARY_DIR})
#link_directories(${DXFLIB_LIBRARY_DIR})
#link_directories(${GEOM_LIBRARY_DIR})

add_library(insightcad SHARED ${insightcad_SOURCES})
target_link_libraries(insightcad toolkit ${OCC_LIBRARIES} ${DXFLIB_LIBRARY} ${GEOM_LIBRARIES})
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

# set(cadtest_SRCS test.cpp)
# add_executable(cadtest ${cadtest_SRCS})
# target_link_libraries(cadtest insightcad)


add_pybindings(cad "cad.i" insightcad)

add_subdirectory(iscad)