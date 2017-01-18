project(isofCleanupParallelRuns)

#find_package(Qt4 COMPONENTS QtCore QtGui QtWebKit REQUIRED)

include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QWT_INCLUDE_DIR})

# set(CMAKE_AUTOMOC ON)
# set(CMAKE_AUTORCC ON)

set(isofCleanupParallelRuns_SRCS main.cpp)
# SET(isofManageFunctionObject_FORMS isofcasebuilderwindow.ui)
# QT4_WRAP_UI(isofManageFunctionObject_FORMS_HEADERS ${isofCaseBuilder_FORMS})
# qt4_automoc(${isofManageFunctionObject_SRCS})
# # SET(isofCaseBuilder_RCCS isofCleanupParallelRuns.qrc)

add_executable(isofCleanupParallelRuns ${isofCleanupParallelRuns_SRCS}) # ${isofManageFunctionObject_FORMS_HEADERS} ${isofManageFunctionObject_RCCS})
target_link_libraries(isofCleanupParallelRuns toolkit #toolkit_gui
# Qt4::QtCore Qt4::QtGui Qt4::QtWebKit
 )

install(TARGETS isofCleanupParallelRuns RUNTIME DESTINATION bin)