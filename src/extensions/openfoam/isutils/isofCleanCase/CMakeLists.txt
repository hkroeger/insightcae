project(isofCleanCase)

set(isofCleanCase_SOURCES main.cpp)

add_executable(isofCleanCase ${isofCleanCase_SOURCES})
target_link_libraries(isofCleanCase ${Boost_LIBRARIES} toolkit)

install(TARGETS isofCleanCase RUNTIME DESTINATION bin)
