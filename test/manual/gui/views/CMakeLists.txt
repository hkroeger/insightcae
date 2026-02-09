project(test_views)



add_executable(test_views
    test_views.cpp
)
target_enable_qt(test_views)
linkToolkitVtk(test_views Onscreen)

target_include_directories(test_views
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_views
    toolkit_gui
    Qt5::Widgets)
