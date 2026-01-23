project(isofParseSolverLog)

set(isofParseSolverLog_SOURCES
  isofparsesolverlog.cpp
)

add_executable(isofParseSolverLog ${isofParseSolverLog_SOURCES})
target_link_libraries(isofParseSolverLog toolkit)
linkToolkitVtk(isofParseSolverLog Offscreen)
setup_gettext(isofParseSolverLog isofParseSolverLog ${isofParseSolverLog_SOURCES})
target_include_directories(isofParseSolverLog
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isofParseSolverLog RUNTIME DESTINATION bin)

