if (INSIGHT_BUILD_ISCAD)

project(insightcad_gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(VTK
   COMPONENTS
   vtkGUISupportQt
   NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
   HINTS ${VTK_ONSCREEN_DIR}
)

set(insightcad_gui_SRCS 
 iscadmetatyperegistrator.cpp
 iscadmetatyperegistrator.h

 iqiscadmodelgenerator.h iqiscadmodelgenerator.cpp
 iqiscadmodelrebuilder.h iqiscadmodelrebuilder.cpp
 iqiscadscriptmodelgenerator.h iqiscadscriptmodelgenerator.cpp

 iqcaditemmodel.h
 iqcaditemmodel.cpp
 iqpickinteractorstyle.h iqpickinteractorstyle.cpp
 iqcadmodel3dviewer.h iqcadmodel3dviewer.cpp
 iqcadtransformationcommand.h iqcadtransformationcommand.cpp
 iqpointpickcommand.h iqpointpickcommand.cpp
 iqvectordirectioncommand.h iqvectordirectioncommand.cpp
 ivtkoccshape.h ivtkoccshape.cpp
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
 postprocactionvisualizers/solidpropertiesvisualizer.cpp

 insightcae_gui.qrc
)


add_library(insightcad_gui SHARED ${insightcad_gui_SRCS})

# --export-all-symbols is not sufficient for signal/slots to work across DLLs
generate_export_header(insightcad_gui)
target_compile_definitions(insightcad_gui PRIVATE insightcad_gui_EXPORTS)

target_link_libraries(insightcad_gui
  insightcad
  toolkit
  ${OCC_GUI_LIBRARIES}
  Qt5::Widgets
  ${DXFLIB_LIBRARIES}
  ${VTK_LIBRARIES}
  )

target_include_directories(insightcad_gui
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
 )

if (INSIGHT_DEVELOPMENT_FILES)
 install(
    TARGETS insightcad_gui EXPORT insightcad_gui_targets
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
 export(EXPORT insightcad_gui_targets
  FILE "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAECADGUITarget.cmake"
  NAMESPACE insight::
 )
 install(EXPORT insightcad_gui_targets
  FILE InsightCAECADGUITarget.cmake
  NAMESPACE insight::
  DESTINATION ${INSIGHT_ConfigPackageLocation}
 )
else()
 install(
    TARGETS insightcad_gui
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
endif()

install(TARGETS insightcad_gui LIBRARY DESTINATION lib)
endif()
