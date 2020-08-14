project(guitest)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(guitest_SRCS
    main.cpp
    mainwindow.cpp
    mainwindow.ui
)

set(guitest_LIBS
    #insightcad_gui
    toolkit toolkit_gui
    Qt5::Widgets
    ${Boost_LIBRARIES}
    gas_dispersion
)


find_package(Qt5Widgets CONFIG REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE_DIR})


if (wt_FOUND)
    message(STATUS "Found wt")

    LIST(APPEND guitest_LIBS
        toolkit_remote
    )
    ADD_DEFINITIONS("-DHAVE_WT")
else()
    message(STATUS "Did not found wt, disabling remote execution functions.")
endif()




add_executable(guitest ${guitest_SRCS})

target_link_libraries(guitest ${guitest_LIBS})

target_include_directories(guitest
    PUBLIC ${QT_INCLUDES}
    PUBLIC ${Boost_INCLUDE_DIR}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS guitest RUNTIME DESTINATION bin)
