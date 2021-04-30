project(istmod)


set(istmod_SOURCES istmod.cpp)

add_executable(istmod ${istmod_SOURCES})
linkToolkitVtk(istmod Offscreen)
install(TARGETS istmod RUNTIME DESTINATION bin)
#install(FILES ${toolkit_HEADERS} DESTINATION include)
