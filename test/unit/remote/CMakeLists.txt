if (wt_FOUND)

    add_executable(detectionBroadcast detectionBroadcast.cpp)
    target_link_libraries(detectionBroadcast toolkit toolkit_remote)
    linkToolkitVtk(detectionBroadcast Offscreen)

    add_test(unit_remote_detectionBroadcast detectionBroadcast)

endif()
