project(istmod)

include_directories(${toolkit_SOURCE_DIR})
link_directories(${toolkit_BIN_DIR})

set(istmod_SOURCES istmod.cpp)

add_executable(istmod ${istmod_SOURCES})
target_link_libraries(istmod ${Boost_LIBRARIES} toolkit)

install(TARGETS istmod RUNTIME DESTINATION bin)
#install(FILES ${toolkit_HEADERS} DESTINATION include)
