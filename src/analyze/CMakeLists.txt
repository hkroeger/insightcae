project(analyze)

find_package(wt
    HINTS /opt/witty
)

set(analyze_SOURCES
    main.cpp
)

set(analyze_LIBS
    toolkit
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

include_directories(${toolkit_SOURCE_DIR})
link_directories(${toolkit_BIN_DIR})



add_executable(analyze ${analyze_SOURCES})
target_link_libraries(analyze ${analyze_LIBS})

install(TARGETS analyze RUNTIME DESTINATION bin)
