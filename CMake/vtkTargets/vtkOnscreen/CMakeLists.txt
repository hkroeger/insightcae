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
        FiltersParallel
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
        RenderingLabel
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
           vtkFiltersParallel
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
           vtkRenderingLabel
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

if (CMAKE_WRAPPER)
  set(CMAKE "${CMAKE_WRAPPER}") # required in MXE cross build, cmake is called through wrapper script there
else()
  set(CMAKE "${CMAKE_COMMAND}")
endif()

if (WIN32)
  set(LIBNAME lib/libvtkDICOM-8.2.0.dll.a)
else()
  set(LIBNAME lib/libvtkDICOM-pvis.so.0.8.15)
endif()

ExternalProject_Add( vtkdicom-onscreen
   GIT_REPOSITORY "https://github.com/dgobbi/vtk-dicom"
   GIT_TAG 6a81888
   INSTALL_DIR ${CMAKE_BINARY_DIR}
   CMAKE_COMMAND ${CMAKE}
   CMAKE_ARGS
     -DBUILD_SHARED_LIBS=ON
     -DBUILD_PYTHON_WRAPPERS=OFF
     -DCMAKE_BUILD_TYPE=Release
     -DVTK_DIR=${VTK_ONSCREEN_DIR}
     -DPython3_ROOT_DIR=${PY3ROOT}
     -DVTK_VERSIONED_INSTALL=OFF
     -DDICOM_CUSTOM_LIBRARY_SUFFIX=pvis
     -DBUILD_PROGRAMS=OFF
     -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

   BUILD_BYPRODUCTS <INSTALL_DIR>/${LIBNAME}
)

ExternalProject_Get_Property( vtkdicom-onscreen INSTALL_DIR )
ExternalProject_Get_Property( vtkdicom-onscreen SOURCE_DIR )

if (WIN32)
  set(VTKDICOMONSCREEN_INCLUDE_DIR ${INSTALL_DIR}/include)
else()
  set(VTKDICOMONSCREEN_INCLUDE_DIR ${INSTALL_DIR}/include/vtk-8.90)
endif()
set(VTKDICOMONSCREEN_LIBRARY ${INSTALL_DIR}/${LIBNAME})

if (WIN32)
  install(DIRECTORY ${INSTALL_DIR}/bin/
    DESTINATION bin
    FILES_MATCHING PATTERN "libvtkDICOM*.dll*")
  install(DIRECTORY ${INSTALL_DIR}/lib/
    DESTINATION lib
    FILES_MATCHING PATTERN "libvtkDICOM*.dll*")
else()
  install(DIRECTORY ${INSTALL_DIR}/lib/
    DESTINATION lib
    FILES_MATCHING PATTERN "libvtkDICOM*.so*")
endif()


IF (WIN32)

    # everything is onscreen in windows
    add_library(vtkOnscreen INTERFACE)
    target_link_libraries(vtkOnscreen INTERFACE ${VTK_LIBRARIES} ${VTKDICOMONSCREEN_LIBRARY})
    add_dependencies(vtkOnscreen vtkdicom-onscreen)
    add_library(vtkOffscreen INTERFACE)
    target_link_libraries(vtkOffscreen INTERFACE ${VTK_LIBRARIES} ${VTKDICOMONSCREEN_LIBRARY})
    add_dependencies(vtkOffscreen vtkdicom-onscreen)

    add_library(vtkHeaders INTERFACE)
    target_include_directories(vtkHeaders INTERFACE ${VTK_INCLUDE_DIRS} $<BUILD_INTERFACE:${VTKDICOMONSCREEN_INCLUDE_DIR}>)
    target_link_libraries(vtkHeaders INTERFACE ${VTK_LIBRARIES} ${VTKDICOMONSCREEN_LIBRARY})

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
