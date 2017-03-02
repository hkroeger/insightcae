project(isresulttool)

include_directories(${toolkit_SOURCE_DIR})
link_directories(${toolkit_BIN_DIR})

set(isresulttool_SOURCES main.cpp)

add_executable(isresulttool ${isresulttool_SOURCES})
target_link_libraries(isresulttool ${Boost_LIBRARIES} toolkit)

install(TARGETS isresulttool RUNTIME DESTINATION bin)
#install(FILES ${toolkit_HEADERS} DESTINATION include)
