if (INSIGHT_BUILD_ISCAD)

project(iscad)

set(iscad_SRCS 
 iscad.cpp
 iscadmainwindow.cpp
 iscadsyntaxhighlighter.cpp
 iscadapplication.cpp
 iscadmodel.cpp

 modelcomponentselectordlg.cpp
 insertfeaturedlg.cpp

 bgparsingthread.cpp
)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qwt REQUIRED)
#find_package(Boost COMPONENTS system filesystem REQUIRED)

#include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
#include_directories(${toolkit_SOURCE_DIR})
# link_directories(${toolkit_BIN_DIR})

SET(iscad_FORMS modelcomponentselectordlg.ui insertfeaturedlg.ui) #newanalysisdlg.ui analysisform.ui)

SET(iscad_RCCS iscad.qrc)

#QT4_WRAP_UI(iscad_FORMS_HEADERS ${iscad_FORMS})
#qt4_automoc(${iscad_SRCS})

add_executable(iscad ${iscad_SRCS} ${iscad_FORMS} ${iscad_RCCS})

target_link_libraries(iscad
  Qt5::Widgets
  toolkit toolkit_gui
  insightcad insightcad_gui
  ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES} ${OPENGL_LIBRARIES}
  ${Boost_LIBRARIES}
  ${QWT_LIBRARY} ${QWT_LIBRARIES}
  ${DXFLIB_LIBRARIES}
  )

target_include_directories(iscad
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${QWT_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS iscad RUNTIME DESTINATION bin)
endif()
