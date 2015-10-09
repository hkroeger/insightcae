project(insightcad)

find_package(OpenCasCade REQUIRED)
find_package(OpenGL)
find_package(DXFlib)
find_package(GEOM)

set(insightcad_SOURCES 
  dxfwriter.cpp 
  parser.cpp 
  datum.cpp 
  sketch.cpp 
  geotest.cpp 
  solidmodel.cpp 
  meshing.cpp 
  featurefilter.cpp 
  feature.cpp 
  freeship_interface.cpp
  evaluation.cpp
  occtools.cpp
)

set(INCLUDE_DIRS "")
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}") #hack
add_subdirectory(quantitycomputers)
add_subdirectory(featurefilters)
add_subdirectory(solidmodels)

# include_directories(${toolkit_SOURCE_DIR})
# include_directories(${Boost_INCLUDE_DIRS})
# include_directories(${OCC_INCLUDE_DIR})
# include_directories(${DXFLIB_INCLUDE_DIR})
# include_directories(${GEOM_INCLUDE_DIRS})

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

if (INSIGHT_BUILD_WORKBENCH)

set(iscad_SRCS iscad.cpp iscadapplication.cpp qoccviewwidget.cpp qoccviewercontext.cpp solidmodeltransient.cpp)
find_package(Qt4 REQUIRED)
find_package(Qwt REQUIRED)
#find_package(Boost COMPONENTS system filesystem REQUIRED)
set(CMAKE_AUTOMOC ON)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${toolkit_SOURCE_DIR})
# link_directories(${toolkit_BIN_DIR})

SET(iscad_FORMS ) #newanalysisdlg.ui analysisform.ui)
QT4_WRAP_UI(workbench_FORMS_HEADERS ${iscad_FORMS})
qt4_automoc(${iscad_SRCS})

add_executable(iscad ${iscad_SRCS} ${iscad_FORMS_HEADERS})
target_link_libraries(iscad insightcad ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES} ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES} toolkit 
  ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} 
  ${QWT_LIBRARY} ${QWT_LIBRARIES}
  ${DXFLIB_LIBRARIES}
  )
endif()

add_pybindings(cad "cad.i" insightcad)
