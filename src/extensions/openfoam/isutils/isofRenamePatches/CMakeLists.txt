project(isofRenamePatches)

set(isofRenamePatches_SOURCES main.cpp)

add_executable(isofRenamePatches ${isofRenamePatches_SOURCES})
linkToolkitVtk(isofRenamePatches Offscreen)
install(TARGETS isofRenamePatches RUNTIME DESTINATION bin)
