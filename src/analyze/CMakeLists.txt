project(analyze)


set(analyze_SOURCES
    main.cpp
)

set(analyze_LIBS
    toolkit_remote
    ${Boost_LIBRARIES}
)

if (wt_FOUND)
    message(STATUS "Found wt")

    LIST(APPEND analyze_SOURCES
        restapi.cpp
        restapi.h
    )
    LIST(APPEND analyze_LIBS
        Wt::Wt Wt::HTTP
    )
    ADD_DEFINITIONS("-DHAVE_WT")
else()
    message(STATUS "Did not found wt, disabling server.")
endif()

add_executable(analyze ${analyze_SOURCES})
target_link_libraries(analyze ${analyze_LIBS})
linkToolkitVtk(analyze Offscreen)

install(TARGETS analyze RUNTIME DESTINATION bin)
