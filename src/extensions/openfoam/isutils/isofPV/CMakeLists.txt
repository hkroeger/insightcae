project(isofPV)


set(isofPV_SRCS
    isofPV.cpp
    )

add_executable(isofPV ${isofPV_SRCS})
target_link_libraries(isofPV toolkit_remote)
linkToolkitVtk(isofPV Offscreen)
install(TARGETS isofPV RUNTIME DESTINATION bin)
