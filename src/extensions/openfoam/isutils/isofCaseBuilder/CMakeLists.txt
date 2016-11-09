project(isofCaseBuilder)

find_package(Qt4 COMPONENTS QtCore QtGui QtWebKit REQUIRED)
find_package(Qwt REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE_DIR})

set(isofCaseBuilder_SRCS main.cpp isofcasebuilderwindow.cpp)
SET(isofCaseBuilder_FORMS isofcasebuilderwindow.ui)
QT4_WRAP_UI(isofCaseBuilder_FORMS_HEADERS ${isofCaseBuilder_FORMS})
qt4_automoc(${isofCaseBuilder_SRCS})
# SET(isofCaseBuilder_RCCS isofCaseBuilder.qrc)

add_executable(isofCaseBuilder ${isofCaseBuilder_SRCS} ${isofCaseBuilder_FORMS_HEADERS} ${isofCaseBuilder_RCCS})
target_link_libraries(isofCaseBuilder toolkit toolkit_gui
 Qt4::QtCore Qt4::QtGui Qt4::QtWebKit
 )

install(TARGETS isofCaseBuilder RUNTIME DESTINATION bin)
