project(guitest)



set(guitest_LIBS
    toolkit_gui
    Qt5::Widgets
    genericmodules
)


if (wt_FOUND)
    message(STATUS "Found wt")

    LIST(APPEND guitest_LIBS
        toolkit_remote
    )
    ADD_DEFINITIONS("-DHAVE_WT")
else()
    message(STATUS "Did not found wt, disabling remote execution functions.")
endif()




add_executable(guitest
    main.cpp
    mainwindow.cpp
    mainwindow.ui
)
target_enable_qt(guitest)
target_link_libraries(guitest ${guitest_LIBS})

linkToolkitVtk(guitest Onscreen)

target_include_directories(guitest
    PUBLIC ${QT_INCLUDES}
    PUBLIC ${Boost_INCLUDE_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS guitest RUNTIME DESTINATION bin)
