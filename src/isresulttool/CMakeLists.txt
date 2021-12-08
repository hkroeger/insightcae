project(isResultTool)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Charts CONFIG REQUIRED)


set(isresulttool_SOURCES
    isresulttool.cpp
    resultviewwindow.cpp resultviewwindow.h resultviewwindow.ui
    iqaddfilterdialog.cpp iqaddfilterdialog.h iqaddfilterdialog.ui
    iqselectresultelementsdialog.h iqselectresultelementsdialog.cpp iqselectresultelementsdialog.ui
    )
SET(isresulttool_RCCS
    resultviewwindow.qrc
    )

add_executable(isResultTool ${isresulttool_SOURCES} ${isresulttool_RCCS})
target_link_libraries(isResultTool
    toolkit_gui
    Qt5::Widgets Qt5::Charts
    )
linkToolkitVtk(isResultTool Onscreen)
target_include_directories(isResultTool
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isResultTool RUNTIME DESTINATION bin)
