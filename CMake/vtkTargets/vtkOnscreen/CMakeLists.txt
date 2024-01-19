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
        #IOParallel # not available in mxe
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
           #IOParallel # not available in mxe
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


# need to handle extensions to VTK here in two versions (onscreen and offscreen)
include(ExternalProject)

# 1) vtk dicom reader
get_filename_component(PY3ROOT ${PYTHON_LIBRARY} DIRECTORY) #lib
get_filename_component(PY3ROOT ${PY3ROOT} DIRECTORY) #root

ExternalProject_Add( vtkdicom-onscreen
   GIT_REPOSITORY "https://github.com/dgobbi/vtk-dicom"
   GIT_TAG 6a81888
   CMAKE_ARGS
     -DBUILD_SHARED_LIBS=ON
     -DBUILD_PYTHON_WRAPPERS=OFF
     -DCMAKE_BUILD_TYPE=Release
     -DVTK_DIR=${VTK_ONSCREEN_DIR}
     -DPython3_ROOT_DIR=${PY3ROOT}
     -DVTK_VERSIONED_INSTALL=OFF
     -DDICOM_CUSTOM_LIBRARY_SUFFIX=pvis
     -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

   BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libvtkDICOM-pvis.so
)

ExternalProject_Get_Property( vtkdicom-onscreen INSTALL_DIR )
ExternalProject_Get_Property( vtkdicom-onscreen SOURCE_DIR )

set(VTKDICOMONSCREEN_INCLUDE_DIR ${INSTALL_DIR}/include/vtk-8.90)
set(VTKDICOMONSCREEN_LIBDIR ${INSTALL_DIR}/lib)
set(VTKDICOMONSCREEN_LIBRARY ${VTKDICOMONSCREEN_LIBDIR}/libvtkDICOM-pvis.so.0.8.15)

install(FILES ${VTKDICOMONSCREEN_LIBRARY} DESTINATION lib)


IF (WIN32)

    # everything is onscreen in windows
    add_library(vtkOnscreen INTERFACE)
    target_link_libraries(vtkOnscreen INTERFACE ${VTK_LIBRARIES} ${VTKDICOMONSCREEN_LIBRARY})
    add_dependencies(vtkOnscreen vtkdicom-onscreen)
    add_library(vtkOffscreen INTERFACE)
    target_link_libraries(vtkOffscreen INTERFACE ${VTK_LIBRARIES} ${VTKDICOMONSCREEN_LIBRARY})
    add_dependencies(vtkOffscreen vtkdicom-onscreen)

    add_library(vtkHeaders INTERFACE)
    target_include_directories(vtkHeaders INTERFACE ${VTK_INCLUDE_DIRS} ${VTKDICOMONSCREEN_INCLUDE_DIR})
    target_link_libraries(vtkHeaders INTERFACE ${VTK_LIBRARIES} ${VTKDICOMONSCREEN_LIBRARY})

else()

    set(LIBS "")

    list(APPEND LIBS
         ${VTKDICOMONSCREEN_LIBRARY}
    )

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
    add_dependencies(vtkOnscreen vtkdicom-onscreen)
    target_link_libraries(vtkOnscreen INTERFACE ${LIBS})

endif()
