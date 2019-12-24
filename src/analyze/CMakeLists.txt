project(analyze)

find_package(wt
    HINTS /opt/witty
)

include_directories(${toolkit_SOURCE_DIR})
link_directories(${toolkit_BIN_DIR})

set(analyze_SOURCES
    main.cpp
    restapi.cpp
    restapi.h
)

add_executable(analyze ${analyze_SOURCES})

target_link_libraries(analyze
    toolkit
    ${Boost_LIBRARIES}
    Wt::Wt Wt::HTTP
)

install(TARGETS analyze RUNTIME DESTINATION bin)
