if (wt_FOUND)

    add_executable(detectionBroadcast detectionBroadcast.cpp)
    target_link_libraries(detectionBroadcast toolkit toolkit_remote)
    linkToolkitVtk(detectionBroadcast Offscreen)

    add_test(unit_remote_detectionBroadcast detectionBroadcast)

endif()


# hermetic tests of the remote execution machinery (see test/remote_common)
if (INSIGHT_REMOTE_TESTS_ENABLED)

    remote_test(remote_serverlist_config  TEST_NAME remote_serverlist_config  LABELS unit)
    remote_test(remote_lookforpattern     TEST_NAME remote_lookforpattern     LABELS unit TIMEOUT 60)
    remote_test(remote_wslversion_parse   TEST_NAME remote_wslversion_parse   LABELS unit TIMEOUT 60)
    remote_test(remote_analyzeclient      TEST_NAME remote_analyzeclient      LABELS unit)
    remote_test(remote_analyzeserver_local TEST_NAME remote_analyzeserver_local LABELS unit TIMEOUT 900)

    if (TARGET workbench_core)
        remote_test(remote_undosteps TEST_NAME remote_undosteps LABELS unit
            LIBS workbench_core)
    endif()

endif()
