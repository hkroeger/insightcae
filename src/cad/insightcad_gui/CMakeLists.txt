if (INSIGHT_BUILD_ISCAD)

project(insightcad_gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)

set(insightcad_gui_SRCS 
 iscadmetatyperegistrator.cpp
 iscadmetatyperegistrator.h

 cadfeaturetransient.cpp
 cadfeaturetransient.h
 pointertransient.cpp
 pointertransient.h
# qoccviewercontext.cpp
# qoccviewercontext.h
 occtwindow.cpp
 occtwindow.h
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
)


add_library(insightcad_gui SHARED ${insightcad_gui_SRCS})

target_link_libraries(insightcad_gui
  insightcad
  toolkit
  ${OCC_LIBRARIES} ${OCC_OCAF_LIBRARIES}
  Qt5::Widgets
  ${DXFLIB_LIBRARIES}
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
