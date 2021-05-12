project(vtkOffscreen)

add_library(vtkHeaders INTERFACE)

if (WIN32)

    # leave vtkOffscreen adn vtkHeaders empty
    # vtkOffscreen target is created elsewhere

else()

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
            CommonDataModel
            CommonMisc
            CommonExecutionModel
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

    if (INSIGHT_BUILD_MEDREADER)
        list(APPEND LIBS
            ${CMAKE_BINARY_DIR}/medreader-offscreen/lib/libvtkMedReader-pvisoffscreen.so
        )
    endif()

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

    get_target_property(vtkOffscreen_INCLUDES VTK::FiltersCore INTERFACE_INCLUDE_DIRECTORIES)

    if (INSIGHT_BUILD_MEDREADER)
        list(APPEND vtkOffscreen_INCLUDES
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/medreader-offscreen/include> $<INSTALL_INTERFACE:paraview-offscreen/include>
        )
    endif()

    foreach (_l ${VTK_LIBRARIES})
        adddeps(${_l})
    endforeach()
    list(REMOVE_DUPLICATES LIBS)
    set(VTK_OFFSCREEN_LIBRARIES "${LIBS}" PARENT_SCOPE)

    add_library(vtkOffscreen INTERFACE)
    target_link_libraries(vtkOffscreen INTERFACE ${LIBS})

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

endif()
