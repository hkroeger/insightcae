project(vtk2key)


add_executable(vtk2key
    vtk2key.cpp
)
linkToolkitVtk(vtk2key Offscreen)
target_setup_gettext(vtk2key vtk2key)

install(TARGETS vtk2key RUNTIME DESTINATION bin)
