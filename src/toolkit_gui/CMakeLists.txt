project(toolkit_gui)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
#find_package(Qt5WebEngineWidgets CONFIG REQUIRED)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qwt REQUIRED)

set(toolkit_gui_SRCS 
    parameterwrapper.cpp 
    parametereditorwidget.cpp
    qdebugstream.cpp
    qinsighterror.cpp
    email.cpp
    mimetypemanager.cpp
    helpwidget.cpp
    of_clean_case.cpp
    snappyhexmesh_gui.cpp
    blockmesh_curvedcylinder.cpp
    blockmesh_cylwedge.cpp
    blockmesh_cylwedgeortho.cpp
    blockmesh_tubemesh.cpp
    blockmesh_gui.cpp
    parametersetvisualizer.cpp
    logviewerwidget.cpp
    graphprogressdisplayer.cpp
    insightcaeapplication.cpp
    remotedirselector.cpp
    remotesync.cpp
    sdmdiarea.cpp
    blockmesh_cylwedge_gui.cpp
    blockmesh_cylwedgeortho_gui.cpp
    metatypes.cpp
    remoteparaview.cpp
    remoteparaview.h
)

file(GLOB_RECURSE toolkit_gui_HEADERS "*.h")

SET(toolkit_gui_FORMS
    of_clean_case.ui
    remotedirselector.ui
    remoteparaview.ui
)
SET(toolkit_gui_RCCS toolkit_gui.qrc)

add_library(toolkit_gui SHARED ${toolkit_gui_SRCS} ${toolkit_gui_HEADERS} ${toolkit_gui_FORMS} ${toolkit_gui_RCCS})
add_PDL(toolkit_gui "${toolkit_gui_HEADERS}")

target_link_libraries(toolkit_gui
 toolkit insightcad_gui
 Qt5::Widgets #Qt5::WebEngineWidgets
 ${QWT_LIBRARY} ${QWT_LIBRARIES}
 ${Boost_LIBRARIES}
 )

target_include_directories(toolkit_gui
  PUBLIC ${QT_INCLUDES} 
  PUBLIC ${QWT_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS toolkit_gui LIBRARY DESTINATION lib)
