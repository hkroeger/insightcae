project(toolkit_gui)

find_package(Qt4 COMPONENTS QtCore QtGui QtWebKit REQUIRED)
find_package(Qwt REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(toolkit_gui_SRCS 
    parameterwrapper.cpp 
    parametereditorwidget.cpp
    qdebugstream.cpp
)
#SET(toolkit_gui_FORMS newanalysisdlg.ui analysisform.ui)
#QT4_WRAP_UI(toolkit_gui_FORMS_HEADERS ${workbench_FORMS})
qt4_automoc(${toolkit_gui_SRCS})
#SET(toolkit_gui_RCCS toolkit_gui.qrc)

add_library(toolkit_gui SHARED ${toolkit_gui_SRCS} ${toolkit_gui_FORMS_HEADERS} ${toolkit_gui_RCCS})
target_link_libraries(toolkit_gui ${Boost_LIBRARIES} toolkit 
 Qt4::QtCore Qt4::QtGui Qt4::QtWebKit
 ${QWT_LIBRARY} ${QWT_LIBRARIES}
 )
target_include_directories(toolkit_gui
  PUBLIC ${QT_INCLUDES} 
  PUBLIC ${QWT_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )
install(TARGETS toolkit_gui LIBRARY DESTINATION lib)
