project(paraviewextensions)

install_package_script(Paraview __init__.py)
#install(files __init__.py destination share/insight/python/Insight/Paraview)

if (MED_DIR)
    list(APPEND VTK_CMAKE_ADDARGS "-DMED3HOME=${MED_DIR}")
endif()
if(HDF_DIR)
    list(APPEND VTK_CMAKE_ADDARGS "-DHDF5_ROOT=${HDF_DIR}")
endif()
if (PYTHON_LIBRARY)
    cmake_path(GET PYTHON_LIBRARY PARENT_PATH PYROOT)
    cmake_path(GET PYROOT PARENT_PATH PYROOT) # two dirs up
    list(APPEND VTK_CMAKE_ADDARGS "-DPython3_ROOT_DIR=${PYROOT}")
endif()
if (INSIGHT_BUILD_MEDREADER)

    include(ExternalProject)

    if (WIN32)
      set(LIBNAME lib/libvtkMedReader-pvis.dll.a)
      list(APPEND VTK_CMAKE_ADDARGS "-DMedReader_BUILD_ParaView_Plugin=OFF")
    else()
      set(LIBNAME lib/libvtkMedReader-pvis.so)
    endif()

    externalproject_add(medreader-onscreen
        DOWNLOAD_COMMAND "" # Disable download
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/MedReader"

        CMAKE_COMMAND ${CMAKE_WRAPPER}
        CMAKE_ARGS
         -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
         -DVTK_DIR=${VTK_ONSCREEN_DIR}
         -DVTKCompileTools_DIR=${VTKCompileTools_DIR}
         -DParaView_DIR=${VTK_ONSCREEN_DIR}/..
         -DLIBRARY_NAME_SUFFIX=pvis
         -DCMAKE_INSTALL_LIBDIR=lib
         ${VTK_CMAKE_ADDARGS}

        INSTALL_DIR ${CMAKE_BINARY_DIR}/medreader-onscreen
        BUILD_BYPRODUCTS <INSTALL_DIR>/${LIBNAME} # required for ninja
    )
  install(DIRECTORY ${CMAKE_BINARY_DIR}/medreader-onscreen/ DESTINATION "." USE_SOURCE_PERMISSIONS)
  add_dependencies(vtkOnscreen medreader-onscreen)

  if (NOT WIN32)
    externalproject_add(medreader-offscreen
        DOWNLOAD_COMMAND "" # Disable download
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/MedReader"

        CMAKE_ARGS
         -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
         -DVTK_DIR=${VTK_OFFSCREEN_DIR}
         -DVTKCompileTools_DIR=${VTKCompileTools_DIR}
         -DParaView_DIR=${VTK_OFFSCREEN_DIR}/..
         -DLIBRARY_NAME_SUFFIX=pvisoffscreen
         -DCMAKE_INSTALL_LIBDIR=lib
         ${VTK_CMAKE_ADDARGS}

        INSTALL_DIR ${CMAKE_BINARY_DIR}/medreader-offscreen
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libvtkMedReader-pvisoffscreen.so # required for ninja
    )
    install(DIRECTORY ${CMAKE_BINARY_DIR}/medreader-offscreen/ DESTINATION "paraview/paraview-offscreen" USE_SOURCE_PERMISSIONS)

    # librarys and include paths are added in
    # vtkOnscreen/vtkOffscreen CMake includes
    add_dependencies(vtkOffscreen medreader-offscreen)
  endif()

endif()

add_subdirectory(vtk2key)


