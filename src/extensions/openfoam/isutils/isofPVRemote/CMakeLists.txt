project(isofPVRemote)


set(isofPVRemote_SRCS
    isofPVRemote.cpp
    )

add_executable(isofPVRemote ${isofPVRemote_SRCS})
target_link_libraries(isofPVRemote toolkit_remote)
linkToolkitVtk(isofPVRemote Offscreen)
install(TARGETS isofPVRemote RUNTIME DESTINATION bin)
