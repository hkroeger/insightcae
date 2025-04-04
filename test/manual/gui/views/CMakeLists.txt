project(test_views)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(SRC
    test_views.cpp
    )

set(LIBS
    toolkit_gui
    Qt5::Widgets
)


find_package(Qt5Widgets CONFIG REQUIRED)

add_executable(test_views ${SRC})
linkToolkitVtk(test_views Onscreen)

target_include_directories(test_views
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(test_views ${LIBS})
