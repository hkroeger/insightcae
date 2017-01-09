project(isofManageFunctionObject)

#find_package(Qt4 COMPONENTS QtCore QtGui QtWebKit REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE_DIR})

# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)

set(isofManageFunctionObject_SRCS main.cpp)
# SET(isofManageFunctionObject_FORMS isofcasebuilderwindow.ui)
# QT4_WRAP_UI(isofManageFunctionObject_FORMS_HEADERS ${isofCaseBuilder_FORMS})
# qt4_automoc(${isofManageFunctionObject_SRCS})
# # SET(isofCaseBuilder_RCCS isofManageFunctionObject.qrc)

add_executable(isofManageFunctionObject ${isofManageFunctionObject_SRCS}) # ${isofManageFunctionObject_FORMS_HEADERS} ${isofManageFunctionObject_RCCS})
target_link_libraries(isofManageFunctionObject toolkit toolkit_gui
# Qt4::QtCore Qt4::QtGui Qt4::QtWebKit
 )

install(TARGETS isofManageFunctionObject RUNTIME DESTINATION bin)