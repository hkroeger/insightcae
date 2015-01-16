project(test_dictParser)

# link_directories(${toolkit_BIN_DIR})

set(test_dictParser_SOURCES test.cpp)

add_executable(test_dictParser ${test_dictParser_SOURCES})
target_link_libraries(test_dictParser ${Boost_LIBRARIES} toolkit)
