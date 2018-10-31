project(toolkit_gui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5WebKit CONFIG REQUIRED)
find_package(Qt5WebKitWidgets CONFIG REQUIRED)
find_package(Qt5WebView CONFIG REQUIRED)
find_package(Qwt REQUIRED)

set(toolkit_gui_SRCS 
    parameterwrapper.cpp 
    parametereditorwidget.cpp
    qdebugstream.cpp
    email.cpp
    mimetypemanager.cpp
)

add_library(toolkit_gui SHARED ${toolkit_gui_SRCS})

target_link_libraries(toolkit_gui
 toolkit insightcad_gui
 Qt5::Widgets Qt5::WebView Qt5::WebKit Qt5::WebKitWidgets
 ${Boost_LIBRARIES}
 ${QWT_LIBRARY} ${QWT_LIBRARIES}
 )

target_include_directories(toolkit_gui
  PUBLIC ${QT_INCLUDES} 
  PUBLIC ${QWT_INCLUDE_DIR}
  PUBLIC ${Boost_INCLUDE_DIR}
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
 )

install(TARGETS toolkit_gui LIBRARY DESTINATION lib)
