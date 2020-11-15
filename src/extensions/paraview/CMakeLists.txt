project(paraviewextensions)

install_package_script(Paraview __init__.py)
#install(files __init__.py destination share/insight/python/Insight/Paraview)


if (INSIGHT_BUILD_MEDREADER)

    include(ExternalProject)

    externalproject_add(medreader-onscreen
        DOWNLOAD_COMMAND "" # Disable download
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/MedReader"

        CMAKE_ARGS
         -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
         -DVTK_DIR=${VTK_ONSCREEN_DIR}
         -DParaView_DIR=${VTK_ONSCREEN_DIR}/..
         -DLIBRARY_NAME_SUFFIX=pvis

        INSTALL_DIR ${CMAKE_BINARY_DIR}/medreader-onscreen
    )

    externalproject_add(medreader-offscreen
        DOWNLOAD_COMMAND "" # Disable download
        SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/MedReader"

        CMAKE_ARGS
         -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
         -DVTK_DIR=${VTK_OFFSCREEN_DIR}
         -DParaView_DIR=${VTK_OFFSCREEN_DIR}/..
         -DLIBRARY_NAME_SUFFIX=pvisoffscreen

        INSTALL_DIR ${CMAKE_BINARY_DIR}/medreader-offscreen
    )

    install(DIRECTORY ${CMAKE_BINARY_DIR}/medreader-onscreen/ DESTINATION "." USE_SOURCE_PERMISSIONS)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/medreader-offscreen/ DESTINATION "paraview-offscreen" USE_SOURCE_PERMISSIONS)

    # librarys and include paths are added in
    # vtkOnscreen/vtkOffscreen CMake includes

endif()

