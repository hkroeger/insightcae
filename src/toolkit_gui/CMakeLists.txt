project(toolkit_gui)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
#find_package(Qt5WebEngineWidgets CONFIG REQUIRED)

set(toolkit_gui_SRCS 
    parameterwrapper.cpp 
    parametereditorwidget.cpp
    qdebugstream.cpp
    email.cpp
    mimetypemanager.cpp
    helpwidget.cpp
    of_clean_case.cpp
    snappyhexmesh_gui.cpp
    blockmesh_curvedcylinder.cpp
    blockmesh_tubemesh.cpp
    blockmesh_gui.cpp
    parametersetvisualizer.cpp
)

file(GLOB_RECURSE toolkit_gui_HEADERS "*.h")

SET(toolkit_gui_FORMS
    of_clean_case.ui
)

add_library(toolkit_gui SHARED ${toolkit_gui_SRCS} ${toolkit_gui_FORMS})
add_PDL(toolkit_gui "${toolkit_gui_HEADERS}")

target_link_libraries(toolkit_gui
 toolkit insightcad_gui
 Qt5::Widgets #Qt5::WebEngineWidgets
 ${Boost_LIBRARIES}
 )

target_include_directories(toolkit_gui
  PUBLIC ${QT_INCLUDES} 
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS toolkit_gui LIBRARY DESTINATION lib)
