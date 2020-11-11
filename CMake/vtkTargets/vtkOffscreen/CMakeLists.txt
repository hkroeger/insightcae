project(vtkOffscreen)

unset(VTK_DIR CACHE)
unset(VTKm_DIR CACHE)
unset(ParaView_DIR CACHE)
find_package(VTK
    COMPONENTS
        FiltersCore
        FiltersGeneral
        FiltersSources
        FiltersGeneric
        FiltersExtraction
        FiltersPoints
        FiltersFlowPaths
        CommonCore
        IOLegacy
        IOGeometry
        IOExport
        IOImage
        IOXML
        ImagingCore
        RenderingOpenGL2
        RenderingAnnotation
         InteractionWidgets
         InteractionStyle
    REQUIRED
    NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
    HINTS ${VTK_OFFSCREEN_DIR}
)

set(LIBS "")

macro(adddeps _target)
    get_target_property(_libs ${_target} INTERFACE_LINK_LIBRARIES)
    get_target_property(_prop ${_target} IMPORTED_LOCATION_RELEASE)
    foreach (_l ${_libs})
 	 if(TARGET ${_l})
	  if (_prop MATCHES ".*-NOTFOUND")
           adddeps(${_l})
	  else()
	   list(APPEND LIBS ${_prop})
	  endif()
         endif()
    endforeach()
endmacro()

add_library(vtkOffscreen INTERFACE)
foreach (_l ${VTK_LIBRARIES})
    adddeps(${_l})
endforeach()
list(REMOVE_DUPLICATES LIBS)

target_link_libraries(vtkOffscreen INTERFACE ${LIBS})

get_target_property(vtkOffscreen_INCLUDES VTK::FiltersCore INTERFACE_INCLUDE_DIRECTORIES)

add_library(vtkHeaders INTERFACE)
target_include_directories(vtkHeaders INTERFACE ${vtkOffscreen_INCLUDES})


install(
    TARGETS vtkHeaders EXPORT vtkHeaders_targets
    LIBRARY DESTINATION lib
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
    )
export(EXPORT vtkHeaders_targets
  FILE "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEvtkHeadersTarget.cmake"
  NAMESPACE insight::
)
install(EXPORT vtkHeaders_targets
  FILE InsightCAEvtkHeadersTarget.cmake
  NAMESPACE insight::
  DESTINATION ${INSIGHT_ConfigPackageLocation}
)
