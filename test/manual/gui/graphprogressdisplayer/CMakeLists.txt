project(test_graphprogressdisplayer)



add_executable(test_graphprogressdisplayer
    main.cpp
    graphprogressdisplayertestwindow.h graphprogressdisplayertestwindow.cpp
    graphprogressdisplayertestwindow.ui
    )
target_enable_qt(test_graphprogressdisplayer)
linkToolkitVtk(test_graphprogressdisplayer Onscreen)

target_include_directories(test_graphprogressdisplayer
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_graphprogressdisplayer
    toolkit_gui
    Qt5::Widgets
)
