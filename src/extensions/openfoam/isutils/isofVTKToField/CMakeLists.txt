project(isofVTKToField)

#set(CMAKE_AUTOMOC ON)
#SET(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTORCC ON)
#find_package(Qt5Widgets CONFIG REQUIRED)

set(isofVTKToField_SOURCES main.cpp)

add_executable(isofVTKToField ${isofVTKToField_SOURCES})
target_link_libraries(isofVTKToField toolkit)

install(TARGETS isofVTKToField RUNTIME DESTINATION bin)
