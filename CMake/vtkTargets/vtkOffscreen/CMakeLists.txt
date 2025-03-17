project(vtkOffscreen)


if (WIN32)

    # everything is onscreen in Windows
    # vtkOffscreen and vtkHeaders target is created along with vtkOnscreen

else()
    add_library(vtkHeaders INTERFACE)

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
            FiltersVerdict
            FiltersParallel
            CommonColor
            CommonCore
            CommonDataModel
            CommonMisc
            CommonExecutionModel
            IOLegacy
            IOGeometry
            IOExport
            IOImage
            IOXML
            #IOParallel # not available in mxe
            ImagingCore
            RenderingLabel
            RenderingOpenGL2
            RenderingAnnotation
             InteractionWidgets
             InteractionStyle

        REQUIRED
        NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH
        HINTS ${VTK_OFFSCREEN_DIR}
    )

    # need to handle extensions to VTK here in two versions (onscreen and offscreen)
    include(ExternalProject)

    # 1) vtk dicom reader
    get_filename_component(PY3ROOT ${PYTHON_LIBRARY} DIRECTORY) #lib
    get_filename_component(PY3ROOT ${PY3ROOT} DIRECTORY) #root


    ExternalProject_Add( vtkdicom-offscreen
       DOWNLOAD_COMMAND "" # Disable download
       SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../vtk-dicom"

       INSTALL_DIR ${CMAKE_BINARY_DIR}

       CMAKE_ARGS
         -DBUILD_SHARED_LIBS=ON
         -DBUILD_PYTHON_WRAPPERS=OFF
         -DCMAKE_BUILD_TYPE=Release
         -DVTK_DIR=${VTK_OFFSCREEN_DIR}
         -DPython3_ROOT_DIR=${PY3ROOT}
         -DVTK_VERSIONED_INSTALL=OFF
         -DDICOM_CUSTOM_LIBRARY_SUFFIX=pvisoffscreen
         -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

       BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libvtkDICOM-pvisoffscreen.so.0.8.15
    )

    ExternalProject_Get_Property( vtkdicom-offscreen INSTALL_DIR )
    ExternalProject_Get_Property( vtkdicom-offscreen SOURCE_DIR )

    set(VTKDICOMOFFSCREEN_INCLUDE_DIR ${INSTALL_DIR}/include/vtk-8.90)
    set(VTKDICOMOFFSCREEN_LIBDIR ${INSTALL_DIR}/lib)
    set(VTKDICOMOFFSCREEN_LIBRARY ${VTKDICOMOFFSCREEN_LIBDIR}/libvtkDICOM-pvisoffscreen.so.0.8.15)

    install(DIRECTORY ${VTKDICOMOFFSCREEN_LIBDIR}/
      DESTINATION lib
      FILES_MATCHING PATTERN "libvtkDICOM*.so*")

    set(LIBS "")

    list(APPEND LIBS
        ${VTKDICOMOFFSCREEN_LIBRARY}
    )

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

    list(APPEND vtkOffscreen_INCLUDES
        $<BUILD_INTERFACE:${VTKDICOMOFFSCREEN_INCLUDE_DIR}>
    )

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
    add_dependencies(vtkOffscreen vtkdicom-offscreen)

    target_include_directories(vtkHeaders INTERFACE ${vtkOffscreen_INCLUDES})
    add_dependencies(vtkHeaders vtkdicom-offscreen)
    # if (INSIGHT_BUILD_MEDREADER)
    #   add_dependencies(vtkHeaders vtkdicom-offscreen)
    # endif()


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
