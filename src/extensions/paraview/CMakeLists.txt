project(paraviewextensions)

install_package_script(Paraview __init__.py)
#install(files __init__.py destination share/insight/python/Insight/Paraview)

set(ADDARGS "")
if (MED_DIR)
    list(APPEND ADDARGS "-DMED3HOME=${MED_DIR}")
endif()
if(HDF_DIR)
    list(APPEND ADDARGS "-DHDF5_ROOT=${HDF_DIR}")
endif()
if (PYTHON_LIBRARY)
    cmake_path(GET PYTHON_LIBRARY PARENT_PATH PYROOT)
    cmake_path(GET PYROOT PARENT_PATH PYROOT) # two dirs up
    list(APPEND ADDARGS "-DPython3_ROOT_DIR=${PYROOT}")
endif()
if (INSIGHT_BUILD_MEDREADER)

    include(ExternalProject)

    externalproject_add(medreader-onscreen
        DOWNLOAD_COMMAND "" # Disable download
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/MedReader"

        CMAKE_ARGS
         -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
         -DVTK_DIR=${VTvi K_ONSCREEN_DIR}
         -DParaView_DIR=${VTK_ONSCREEN_DIR}/..
         -DLIBRARY_NAME_SUFFIX=pvis
         ${ADDARGS}

        INSTALL_DIR ${CMAKE_BINARY_DIR}/medreader-onscreen
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libvtkMedReader-pvis.so # required for ninja
    )

    externalproject_add(medreader-offscreen
        DOWNLOAD_COMMAND "" # Disable download
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/MedReader"

        CMAKE_ARGS
         -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
         -DVTK_DIR=${VTK_OFFSCREEN_DIR}
         -DParaView_DIR=${VTK_OFFSCREEN_DIR}/..
         -DLIBRARY_NAME_SUFFIX=pvisoffscreen
         ${ADDARGS}

        INSTALL_DIR ${CMAKE_BINARY_DIR}/medreader-offscreen
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libvtkMedReader-pvisoffscreen.so # required for ninja
    )

    install(DIRECTORY ${CMAKE_BINARY_DIR}/medreader-onscreen/ DESTINATION "." USE_SOURCE_PERMISSIONS)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/medreader-offscreen/ DESTINATION "paraview-offscreen" USE_SOURCE_PERMISSIONS)

    # librarys and include paths are added in
    # vtkOnscreen/vtkOffscreen CMake includes
    add_dependencies(vtkOnscreen medreader-onscreen)
    add_dependencies(vtkOffscreen medreader-offscreen)

endif()


