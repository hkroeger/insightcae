project(analyze)

include_directories(${toolkit_SOURCE_DIR})
link_directories(${toolkit_BIN_DIR})

set(analyze_SOURCES main.cpp)

add_executable(analyze ${analyze_SOURCES})
target_link_libraries(analyze ${Boost_LIBRARIES} toolkit)

install(TARGETS analyze RUNTIME DESTINATION bin)
#install(FILES ${toolkit_HEADERS} DESTINATION include)