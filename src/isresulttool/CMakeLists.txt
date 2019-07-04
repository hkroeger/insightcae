project(isresulttool)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qwt REQUIRED)

include_directories(${toolkit_SOURCE_DIR})
link_directories(${toolkit_BIN_DIR})

set(isresulttool_SOURCES main.cpp)
SET(isresulttool_FORMS
    )
SET(isresulttool_RCCS
    #isofplottabular.qrc
    )

add_executable(isresulttool ${isresulttool_SOURCES} ${isresulttool_FORMS} ${isresulttool_RCCS})
target_link_libraries(isresulttool
    ${Boost_LIBRARIES} toolkit
    ${QWT_LIBRARY} ${QWT_LIBRARIES}
    Qt5::Widgets
    )
target_include_directories(isresulttool
  PUBLIC ${QT_INCLUDES}
  PUBLIC ${QWT_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS isresulttool RUNTIME DESTINATION bin)
#install(FILES ${toolkit_HEADERS} DESTINATION include)
