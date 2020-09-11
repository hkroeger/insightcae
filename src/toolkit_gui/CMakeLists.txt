project(toolkit_gui)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Charts CONFIG REQUIRED)

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
    progressrelay.cpp
    insightcaeapplication.cpp
    remotedirselector.cpp
    remotesync.cpp
    sdmdiarea.cpp
    blockmesh_cylwedge_gui.cpp
    blockmesh_cylwedgeortho_gui.cpp
    metatypes.cpp
    remoteparaview.cpp
    remoteparaview.h

    qanalysisthread.cpp
    qanalysisthread.h
    qactionprogressdisplayerwidget.cpp
    qactionprogressdisplayerwidget.h

    qresultsetmodel.cpp
    qresultsetmodel.h
    qresultelements/qcomment.cpp
    qresultelements/qcomment.h
    qresultelements/qimage.cpp
    qresultelements/qimage.h
    qresultelements/qchart.cpp
    qresultelements/qchart.h
    qresultelements/qattributeresulttable.cpp
    qresultelements/qattributeresulttable.h
    qresultelements/qscalarresult.cpp
    qresultelements/qscalarresult.h
    qresultelements/qtabularresult.cpp
    qresultelements/qtabularresult.h
    qresultelements/qvectorresult.cpp
    qresultelements/qvectorresult.h
    qresultelements/qresultsection.cpp
    qresultelements/qresultsection.h
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
 Qt5::Widgets Qt5::Charts
 ${Boost_LIBRARIES}
 )

target_include_directories(toolkit_gui
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
 )

install(
    TARGETS toolkit_gui EXPORT toolkit_gui_targets
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
export(EXPORT toolkit_gui_targets
  FILE "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEToolkitGUITarget.cmake"
  NAMESPACE insight::
)
install(EXPORT toolkit_gui_targets
  FILE InsightCAEToolkitGUITarget.cmake
  NAMESPACE insight::
  DESTINATION ${INSIGHT_ConfigPackageLocation}
)

install(TARGETS toolkit_gui LIBRARY DESTINATION lib)
