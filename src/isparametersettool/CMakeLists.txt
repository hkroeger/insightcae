project(isParameterSetTool)



add_executable(isParameterSetTool
    isparametersettool.cpp
)
target_link_libraries(isParameterSetTool
    toolkit
    )
linkToolkitVtk(isParameterSetTool Offscreen)
target_setup_gettext(isParameterSetTool isParameterSetTool)
target_include_directories(isParameterSetTool
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isParameterSetTool RUNTIME DESTINATION bin)
