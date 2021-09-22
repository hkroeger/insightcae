project(test_graphprogressdisplayer)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SRC
    main.cpp
    graphprogressdisplayertestwindow.h graphprogressdisplayertestwindow.cpp graphprogressdisplayertestwindow.ui
    )

set(LIBS
    toolkit_gui
    Qt5::Widgets
)


find_package(Qt5Widgets CONFIG REQUIRED)

add_executable(test_graphprogressdisplayer ${SRC})
linkToolkitVtk(test_graphprogressdisplayer Onscreen)

target_include_directories(test_graphprogressdisplayer
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_graphprogressdisplayer ${LIBS})
