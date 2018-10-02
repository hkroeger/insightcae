if (INSIGHT_BUILD_ISCAD)

project(insightcad_gui)

set(insightcad_gui_SRCS 
 cadfeaturetransient.cpp
 pointertransient.cpp
 qoccviewercontext.cpp
 qoccviewwidget.cpp
 viewstate.cpp
 
 qvariableitem.cpp
 qmodelstepitem.cpp
 qdatumitem.cpp
 qevaluationitem.cpp
 
 qmodeltree.cpp
)
find_package(Qt4 REQUIRED)
#find_package(Boost COMPONENTS system filesystem REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

#include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
#include_directories(${toolkit_SOURCE_DIR})
# link_directories(${toolkit_BIN_DIR})

qt4_automoc(${insightcad_gui_SRCS})

add_library(insightcad_gui SHARED ${insightcad_gui_SRCS})
target_link_libraries(insightcad_gui insightcad ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES} ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES} toolkit
  Qt4::QtCore Qt4::QtGui Qt4::QtWebKit
  ${QWT_LIBRARY} ${QWT_LIBRARIES}
  ${DXFLIB_LIBRARIES}
  )
target_include_directories(insightcad_gui
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS insightcad_gui LIBRARY DESTINATION lib)
endif()
