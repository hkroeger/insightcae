project(isParameterSetTool)



set(isparametersettool_SOURCES
    isparametersettool.cpp
    )


add_executable(isParameterSetTool ${isparametersettool_SOURCES})
target_link_libraries(isParameterSetTool
    toolkit
    )
linkToolkitVtk(isParameterSetTool Offscreen)
setup_gettext(isParameterSetTool isParameterSetTool ${isparametersettool_SOURCES})
target_include_directories(isParameterSetTool
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isParameterSetTool RUNTIME DESTINATION bin)
