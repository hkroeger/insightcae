project(vtkOnscreen)

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
    HINTS ${VTK_ONSCREEN_DIR}
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


foreach (_l ${VTK_LIBRARIES})
    adddeps(${_l})
endforeach()
list(REMOVE_DUPLICATES LIBS)


add_library( vtkOnscreen INTERFACE )
target_link_libraries(vtkOnscreen INTERFACE ${LIBS})
