if (INSIGHT_BUILD_ISCAD)

project(toolkit_cad)

set(toolkit_cad_SRCS
    cadexception.h cadexception.cpp
    blockmesh_curvedcylinder.cpp blockmesh_curvedcylinder.h
    blockmesh_cylwedgeortho.cpp blockmesh_cylwedgeortho.h
    blockmesh_tubemesh.cpp blockmesh_tubemesh.h
    blockmesh_cylwedge.cpp blockmesh_cylwedge.h
    )

add_library(toolkit_cad SHARED ${toolkit_cad_SRCS})
add_PDL(toolkit_cad "${toolkit_cad_SRCS}")
target_link_libraries(toolkit_cad insightcad toolkit)
target_include_directories(toolkit_cad
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:include/insightcae>
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
 )


if (INSIGHT_DEVELOPMENT_FILES)
 install(
    TARGETS toolkit_cad EXPORT toolkit_cad_targets
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
 export(EXPORT toolkit_cad_targets
  FILE "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEToolkitCADTarget.cmake"
  NAMESPACE insight::
 )
 install(EXPORT toolkit_cad_targets
  FILE InsightCAEToolkitCADTarget.cmake
  NAMESPACE insight::
  DESTINATION ${INSIGHT_ConfigPackageLocation}
 )
else()
 install(
    TARGETS toolkit_cad
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
endif()

endif()
