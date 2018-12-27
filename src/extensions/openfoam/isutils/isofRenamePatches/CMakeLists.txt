project(isofRenamePatches)

#set(CMAKE_AUTOMOC ON)
#SET(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTORCC ON)
#find_package(Qt5Widgets CONFIG REQUIRED)

set(isofRenamePatches_SOURCES main.cpp)

add_executable(isofRenamePatches ${isofRenamePatches_SOURCES})
target_link_libraries(isofRenamePatches ${Boost_LIBRARIES} toolkit #Qt5::Widgets toolkit_gui
    )

install(TARGETS isofRenamePatches RUNTIME DESTINATION bin)
