project(toolkit_gui)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Charts CONFIG REQUIRED)
find_package(Qt5Network CONFIG REQUIRED)
#find_package(Qt5Test CONFIG REQUIRED)

set(toolkit_gui_SRCS 
    parametereditorwidget.cpp

    iqremoteexecutionstate.cpp
    iqremoteexecutionstate.h

    iqparametersetmodel.cpp
    iqresultsetfiltermodel.h iqresultsetfiltermodel.cpp
    iqparameter.cpp
    iqparameters/iqintparameter.cpp
    iqparameters/iqdoubleparameter.cpp
    iqparameters/iqvectorparameter.cpp
    iqparameters/iqstringparameter.cpp
    iqparameters/iqboolparameter.cpp
    iqparameters/iqpathparameter.cpp
    iqparameters/iqmatrixparameter.cpp
    iqparameters/iqdirectoryparameter.cpp
    iqparameters/iqselectionparameter.cpp
    iqparameters/iqarrayparameter.cpp
    iqparameters/iqselectablesubsetparameter.cpp
    iqparameters/iqdoublerangeparameter.cpp
    iqparameters/iqarrayelementparameter.cpp
    iqparameters/iqpropertylibraryselectionparameter.h iqparameters/iqpropertylibraryselectionparameter.cpp

    iqsupplementedinputdatamodel.h iqsupplementedinputdatamodel.cpp

    iqremoteservereditdialog.h
    iqremoteservereditdialog.cpp
    iqremoteservereditdialog.ui

    iqselectremotehosttypedialog.h
    iqselectremotehosttypedialog.cpp
    iqselectremotehosttypedialog.ui

    iqremoteserverlistmodel.h
    iqremoteserverlistmodel.cpp

    iqdebugstream.cpp iqdebugstream.h
    qinsighterror.cpp qinsighterror.h
    email.cpp email.h
    mimetypemanager.cpp mimetypemanager.h
    helpwidget.cpp helpwidget.h
    of_clean_case.cpp of_clean_case.h of_clean_case.ui
    snappyhexmesh_gui.cpp snappyhexmesh_gui.h
    parametersetvisualizer.cpp parametersetvisualizer.h
    logviewerwidget.cpp logviewerwidget.h
    graphprogressdisplayer.cpp graphprogressdisplayer.h
    progressrelay.cpp progressrelay.h
    insightcaeapplication.cpp insightcaeapplication.h
    iqremotefoldermodel.h iqremotefoldermodel.cpp
    remotedirselector.cpp remotedirselector.h remotedirselector.ui
    remotesync.cpp remotesync.h
    sdmdiarea.cpp sdmdiarea.h
    blockmesh_gui.cpp blockmesh_gui.h
    blockmesh_cylwedge_gui.cpp blockmesh_cylwedge_gui.h
    #blockmesh_cylwedgeortho_gui.cpp
    metatypes.cpp metatypes.h
    iqparaviewdialog.h iqparaviewdialog.cpp iqparaviewdialog.ui
    iqremoteparaviewdialog.cpp iqremoteparaviewdialog.h iqremoteparaviewdialog.ui
    iqfiledownloader.h iqfiledownloader.cpp
    iqwaitanimation.h iqwaitanimation.cpp
    analysisguilibrary.h analysisguilibrary.cpp
    wslinstallation.h wslinstallation.cpp
    iqexternalprogramsmodel.h iqexternalprogramsmodel.cpp

    iqconfigureexternalprogramsdialog.h iqconfigureexternalprogramsdialog.cpp iqconfigureexternalprogramsdialog.ui

    iqsetupwsldistributionwizard.h iqsetupwsldistributionwizard.cpp iqsetupwsldistributionwizard.ui

    qanalysisthread.cpp qanalysisthread.h
    qactionprogressdisplayerwidget.cpp qactionprogressdisplayerwidget.h

    iqresultsetmodel.cpp iqresultsetmodel.h
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



SET(toolkit_gui_RCCS toolkit_gui.qrc)

add_library(toolkit_gui SHARED ${toolkit_gui_SRCS} ${toolkit_gui_RCCS})
add_PDL(toolkit_gui "${toolkit_gui_SRCS}")

# --export-all-symbols is not sufficient for signal/slots to work across DLLs
include(GenerateExportHeader)
generate_export_header(toolkit_gui)
target_compile_definitions(toolkit_gui PRIVATE toolkit_gui_EXPORTS)

target_link_libraries(toolkit_gui
 toolkit toolkit_cad insightcad_gui toolkit_remote
 Qt5::Widgets Qt5::Charts Qt5::Network
 ${Boost_LIBRARIES}
 )

target_include_directories(toolkit_gui
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
 )

if (INSIGHT_DEVELOPMENT_FILES)
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
else()
 install(
    TARGETS toolkit_gui
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
endif()

install(TARGETS toolkit_gui LIBRARY DESTINATION lib)
