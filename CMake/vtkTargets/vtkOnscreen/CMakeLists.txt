project(vtkOnscreen)

unset(VTK_DIR CACHE)
unset(VTKm_DIR CACHE)
unset(ParaView_DIR CACHE)

if (NOT WIN32)
 find_package(VTK
    COMPONENTS
        FiltersCore
        FiltersGeneral
        FiltersSources
        FiltersGeneric
        FiltersExtraction
        FiltersPoints
        FiltersFlowPaths
        FiltersVerdict
        CommonCore
        CommonDataModel
        CommonMisc
        CommonExecutionModel
        IOLegacy
        IOGeometry
        IOExport
        IOImage
        IOXML
        IOParallel
        ImagingCore
        GUISupportQt
        RenderingOpenGL2
        RenderingAnnotation
         InteractionWidgets
         InteractionStyle
#    REQUIRED
    NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
    HINTS ${VTK_ONSCREEN_DIR}
 )
else()
    find_package(VTK
       COMPONENTS
           vtkFiltersCore
           vtkFiltersGeneral
           vtkFiltersSources
           vtkFiltersGeneric
           vtkFiltersExtraction
           vtkFiltersPoints
           vtkFiltersFlowPaths
           vtkFiltersVerdict
           vtkCommonCore
           vtkCommonDataModel
           vtkCommonMisc
           vtkCommonExecutionModel
           vtkIOLegacy
           vtkIOGeometry
           vtkIOExport
           vtkIOImage
           vtkIOXML
           IOParallel
           vtkImagingCore
           vtkGUISupportQt
           vtkRenderingOpenGL2
           vtkRenderingAnnotation
            vtkInteractionWidgets
            vtkInteractionStyle
   #    REQUIRED
       NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
       HINTS ${VTK_ONSCREEN_DIR}
    )
endif()


IF (WIN32)

    # everything is onscreen in windows
    add_library(vtkOnscreen INTERFACE)
    target_link_libraries(vtkOnscreen INTERFACE ${VTK_LIBRARIES})
    add_library(vtkOffscreen INTERFACE)
    target_link_libraries(vtkOffscreen INTERFACE ${VTK_LIBRARIES})

    add_library(vtkHeaders INTERFACE)
    target_include_directories(vtkHeaders INTERFACE ${VTK_INCLUDE_DIRS})
    target_link_libraries(vtkHeaders INTERFACE ${VTK_LIBRARIES})

else()

    set(LIBS "")

    if (INSIGHT_BUILD_MEDREADER)
        list(APPEND LIBS
            ${CMAKE_BINARY_DIR}/medreader-onscreen/lib/libvtkMedReader-pvis.so
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

    foreach (_l ${VTK_LIBRARIES})
        adddeps(${_l})
    endforeach()
    list(REMOVE_DUPLICATES LIBS)
    set(VTK_ONSCREEN_LIBRARIES "${LIBS}" PARENT_SCOPE)

    add_library(vtkOnscreen INTERFACE)
    target_link_libraries(vtkOnscreen INTERFACE ${LIBS})

endif()
