project(isResultTool)


add_executable(isResultTool
    isresulttool.cpp
    resultviewwindow.cpp resultviewwindow.h resultviewwindow.ui
    resultviewwindow.qrc
)
find_package(Qt5Charts CONFIG REQUIRED)
target_enable_qt(isResultTool)
target_link_libraries(isResultTool
    toolkit_gui
    Qt5::Widgets Qt5::Charts
    )
linkToolkitVtk(isResultTool Onscreen)
target_include_directories(isResultTool
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isResultTool RUNTIME DESTINATION bin)
