project(vtk2key)


set(vtk2key_SOURCES
    vtk2key.cpp
)


add_executable(vtk2key ${vtk2key_SOURCES})
linkToolkitVtk(vtk2key Offscreen)
setup_gettext(vtk2key vtk2key ${vtk2key_SOURCES})

install(TARGETS vtk2key RUNTIME DESTINATION bin)
