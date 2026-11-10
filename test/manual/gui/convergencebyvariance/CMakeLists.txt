project(test_convergencebyvariance)

find_package(Qt5Charts CONFIG REQUIRED)

add_executable(test_convergencebyvariance
    main.cpp
    casereader.h casereader.cpp
    evaluate.h evaluate.cpp
    convergencewindow.h convergencewindow.cpp
    casewidget.h casewidget.cpp
    statustabwidget.h statustabwidget.cpp
    convergenceplotwidget.h convergenceplotwidget.cpp
    )
target_enable_qt(test_convergencebyvariance)
linkToolkitVtk(test_convergencebyvariance Onscreen)

target_include_directories(test_convergencebyvariance
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_convergencebyvariance
    toolkit_gui
    Qt5::Widgets
    Qt5::Charts
)
