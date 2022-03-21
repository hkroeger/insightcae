if (PYTHONLIBS_FOUND)

    project(test_refdata)

    add_executable(test_refdata test_refdata.cpp)
    target_link_libraries(test_refdata refdata)
    linkToolkitVtk(test_refdata Offscreen)
    add_test(NAME test_refdata COMMAND test_refdata)

endif()
