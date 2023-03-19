project(toolkit_gui)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Charts CONFIG REQUIRED)
find_package(Qt5Network CONFIG REQUIRED)
#find_package(Qt5Test CONFIG REQUIRED)
find_package(OCC REQUIRED)
find_package(VTK
   COMPONENTS
   vtkGUISupportQt vtkRenderingLabel
   NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
   HINTS ${VTK_ONSCREEN_DIR}
)

set(toolkit_gui_SRCS 

 iscadmetatyperegistrator.cpp
 iscadmetatyperegistrator.h

 dialogs/defineplanedialog.h dialogs/defineplanedialog.cpp dialogs/defineplanedialog.ui

 iqiscadmodelgenerator.h iqiscadmodelgenerator.cpp
 iqiscadmodelrebuilder.h iqiscadmodelrebuilder.cpp
 iqiscadscriptmodelgenerator.h iqiscadscriptmodelgenerator.cpp

 iqcaditemmodel.h
 iqcaditemmodel.cpp
 iqcaditemmodel_contextmenu.cpp
 iqcaditemmodel_featurecreation.cpp

 iqpickinteractorstyle.h iqpickinteractorstyle.cpp

 iqcadmodel3dviewer.h iqcadmodel3dviewer.cpp

 iqvtkcadmodel3dviewerrotation.h iqvtkcadmodel3dviewerrotation.cpp
 iqvtkcadmodel3dviewerpanning.h iqvtkcadmodel3dviewerpanning.cpp
 iqvtkcadmodel3dviewer_highlightitem.cpp
 iqvtkcadmodel3dviewer_subshapeselection.cpp
 iqvtkcadmodel3dviewer_clippingplanes.cpp
 iqvtkcadmodel3dviewer.h iqvtkcadmodel3dviewer.cpp

 iqvtkviewerstate.h iqvtkviewerstate.cpp
 iqvtkconstrainedsketcheditor.h iqvtkconstrainedsketcheditor.cpp

 iqvtkvieweractions/iqvtkcadmodel3dviewermeasurepoints.h
 iqvtkvieweractions/iqvtkcadmodel3dviewermeasurepoints.cpp
 iqvtkvieweractions/iqvtkcadmodel3dviewerdrawline.h iqvtkvieweractions/iqvtkcadmodel3dviewerdrawline.cpp

 iqvtkviewwidgetinsertids.h iqvtkviewwidgetinsertids.cpp
 iqcadtransformationcommand.h iqcadtransformationcommand.cpp
 iqpointpickcommand.h iqpointpickcommand.cpp
 iqvectordirectioncommand.h iqvectordirectioncommand.cpp
 iqvtkiscadmodeldisplay.h iqvtkiscadmodeldisplay.cpp

 cadfeaturetransient.cpp
 cadfeaturetransient.h
 pointertransient.cpp
 pointertransient.h
# qoccviewercontext.cpp
# qoccviewercontext.h
 occtwindow.cpp
 occtwindow.h
 occguitools.h occguitools.cpp
 viewwidgetaction.h
 viewwidgetaction.cpp
 viewwidgetinsertids.h
 viewwidgetinsertids.cpp
 navigationmanager.h
 navigationmanager.cpp
 qoccviewwidget.cpp
 qoccviewwidget.h
 viewstate.cpp
 viewstate.h

 qvariableitem.cpp
 qvariableitem.h
 qmodelstepitem.cpp
 qmodelstepitem.h
 qdatumitem.cpp
 qdatumitem.h
 qevaluationitem.cpp
 qevaluationitem.h

 qmodeltree.cpp
 qmodeltree.h


 postprocactionvisualizer.h postprocactionvisualizer.cpp
 postprocactionvisualizers/hydrostaticsvisualizer.cpp
 postprocactionvisualizers/distancevisualizer.cpp
 postprocactionvisualizers/anglevisualizer.cpp
 postprocactionvisualizers/solidpropertiesvisualizer.cpp

 insightcae_gui.qrc

    parametereditorwidget.cpp
    iqoccparametersetdisplay.h iqoccparametersetdisplay.cpp
    iqvtkparametersetdisplay.h iqvtkparametersetdisplay.cpp

    iqremoteexecutionstate.h iqremoteexecutionstate.cpp

    iqparametersetmodel.h iqparametersetmodel.cpp
    iqresultsetfiltermodel.h iqresultsetfiltermodel.cpp
    iqparameter.cpp iqparameter.h
    iqparameters/iqintparameter.cpp iqparameters/iqintparameter.h
    iqparameters/iqdoubleparameter.cpp iqparameters/iqdoubleparameter.h
    iqparameters/iqvectorparameter.cpp iqparameters/iqvectorparameter.h
    iqparameters/iqstringparameter.cpp iqparameters/iqstringparameter.h
    iqparameters/iqboolparameter.cpp iqparameters/iqboolparameter.h
    iqparameters/iqpathparameter.cpp iqparameters/iqpathparameter.h
    iqparameters/iqmatrixparameter.cpp iqparameters/iqmatrixparameter.h
    iqparameters/iqdirectoryparameter.cpp iqparameters/iqdirectoryparameter.h
    iqparameters/iqselectionparameter.cpp iqparameters/iqselectionparameter.h
    iqparameters/iqarrayparameter.cpp iqparameters/iqarrayparameter.h
    iqparameters/iqselectablesubsetparameter.cpp iqparameters/iqselectablesubsetparameter.h
    iqparameters/iqdoublerangeparameter.cpp iqparameters/iqdoublerangeparameter.h
    iqparameters/iqarrayelementparameter.cpp iqparameters/iqarrayelementparameter.h
    iqparameters/iqpropertylibraryselectionparameter.cpp iqparameters/iqpropertylibraryselectionparameter.h
    iqparameters/iqspatialtransformationparameter.cpp iqparameters/iqspatialtransformationparameter.h
    iqparameters/iqcadgeometryparameter.h iqparameters/iqcadgeometryparameter.cpp

    iqsupplementedinputdatamodel.h iqsupplementedinputdatamodel.cpp

    iqremoteservereditdialog.h
    iqremoteservereditdialog.cpp
    iqremoteservereditdialog.ui

    iqselectremotehosttypedialog.h
    iqselectremotehosttypedialog.cpp
    iqselectremotehosttypedialog.ui

    iqremoteserverlistmodel.h
    iqremoteserverlistmodel.cpp

    iqaddfilterdialog.cpp iqaddfilterdialog.h iqaddfilterdialog.ui
    iqselectresultelementsdialog.h iqselectresultelementsdialog.cpp iqselectresultelementsdialog.ui
    iqresultsetdisplayerwidget.h iqresultsetdisplayerwidget.cpp iqresultsetdisplayerwidget.ui

    iqdebugstream.cpp iqdebugstream.h
    qinsighterror.cpp qinsighterror.h
    email.cpp email.h
    mimetypemanager.cpp mimetypemanager.h
    helpwidget.cpp helpwidget.h
    of_clean_case.cpp of_clean_case.h of_clean_case.ui
    snappyhexmesh_gui.cpp snappyhexmesh_gui.h
    cadparametersetvisualizer.cpp cadparametersetvisualizer.h
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
    iqglobalconfigurationmodel.h
    iqmanagereporttemplatesdialog.h iqmanagereporttemplatesdialog.cpp iqmanagereporttemplatesdialog.ui

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
 toolkit toolkit_cad insightcad toolkit_remote
 Qt5::Widgets Qt5::Charts Qt5::Network
 ${Boost_LIBRARIES}
 ${OCC_GUI_LIBRARIES}
 ${DXFLIB_LIBRARIES}
 ${VTK_LIBRARIES}
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
