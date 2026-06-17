
if (INSIGHT_BUILD_COOLPROP AND NOT WIN32)

    include(ExternalProject)
    ExternalProject_Add( coolprop
       DOWNLOAD_COMMAND ""
       SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/CoolProp"

       CMAKE_ARGS
         -DCOOLPROP_SHARED_LIBRARY=ON
         -DCOOLPROP_RELEASE=ON
         -DCMAKE_BUILD_TYPE=Release
         -DCOOLPROP_INSTALL_PREFIX=<INSTALL_DIR>

       BUILD_BYPRODUCTS
          <INSTALL_DIR>/shared_library/CoolPropLib.h
          <INSTALL_DIR>/shared_library/Linux/64bit/libCoolProp.so
    )

    ExternalProject_Get_Property( coolprop INSTALL_DIR )
    ExternalProject_Get_Property( coolprop SOURCE_DIR )
    set(COOLPROP_BUILD_LIBRARY_DIR ${INSTALL_DIR}/shared_library/Linux/64bit)
    set(COOLPROP_BUILD_LIBRARY ${COOLPROP_BUILD_LIBRARY_DIR}/libCoolProp.so)
    set(COOLPROP_SRC_INCLUDE_DIR ${SOURCE_DIR}/include)
    set(COOLPROP_LIBRARY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libCoolProp.so)

    set(COOLPROP_INCLUDE_DIR ${INSTALL_DIR}/include)
    set(COOLPROP_MAIN_HEADER ${COOLPROP_INCLUDE_DIR}/CoolProp.h)

    file(MAKE_DIRECTORY ${COOLPROP_INCLUDE_DIR})

    add_custom_command(
        DEPENDS ${INSTALL_DIR}/shared_library/CoolPropLib.h

        OUTPUT ${COOLPROP_MAIN_HEADER}

        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${COOLPROP_BUILD_LIBRARY_DIR} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${COOLPROP_SRC_INCLUDE_DIR} ${COOLPROP_INCLUDE_DIR}

    )

    #install(FILES ${COOLPROP_BUILD_LIBRARY} DESTINATION lib)
    install(DIRECTORY ${COOLPROP_BUILD_LIBRARY_DIR}/ DESTINATION lib FILES_MATCHING PATTERN "lib*.so*")

    add_library( libcoolprop INTERFACE ${COOLPROP_MAIN_HEADER})
    add_dependencies(libcoolprop coolprop)
    target_link_libraries(libcoolprop INTERFACE ${COOLPROP_BUILD_LIBRARY}) #${COOLPROP_LIBRARY})
    set_property( TARGET libcoolprop PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${COOLPROP_INCLUDE_DIR})

    add_definitions(-DNO_FMTLIB)

    add_executable(coolprop_test coolprop_test.cpp)
    target_link_libraries(coolprop_test libcoolprop)

endif()
