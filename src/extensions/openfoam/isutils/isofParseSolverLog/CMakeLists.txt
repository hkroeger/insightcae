project(isofParseSolverLog)


add_executable(isofParseSolverLog
  isofparsesolverlog.cpp
)
target_link_libraries(isofParseSolverLog toolkit)
linkToolkitVtk(isofParseSolverLog Offscreen)
target_setup_gettext(isofParseSolverLog isofParseSolverLog)
target_include_directories(isofParseSolverLog
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isofParseSolverLog RUNTIME DESTINATION bin)

