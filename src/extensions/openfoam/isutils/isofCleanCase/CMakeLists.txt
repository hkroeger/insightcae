project(isofCleanCase)

set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets CONFIG REQUIRED)

set(isofCleanCase_SOURCES main.cpp)

add_executable(isofCleanCase ${isofCleanCase_SOURCES})
target_link_libraries(isofCleanCase Qt5::Widgets toolkit_gui)
linkToolkitVtk(isofCleanCase Onscreen)
install(TARGETS isofCleanCase RUNTIME DESTINATION bin)
