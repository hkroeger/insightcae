project(isofManageFunctionObject)


include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(isofManageFunctionObject_SRCS main.cpp)

add_executable(isofManageFunctionObject ${isofManageFunctionObject_SRCS}) # ${isofManageFunctionObject_FORMS_HEADERS} ${isofManageFunctionObject_RCCS})
target_link_libraries(isofManageFunctionObject
    toolkit_gui
 )
linkToolkitVtk(isofManageFunctionObject Onscreen)
install(TARGETS isofManageFunctionObject RUNTIME DESTINATION bin)
