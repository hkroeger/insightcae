project(isofVTKToField)


set(isofVTKToField_SOURCES main.cpp)

add_executable(isofVTKToField ${isofVTKToField_SOURCES})
linkToolkitVtk(isofVTKToField Onscreen)

install(TARGETS isofVTKToField RUNTIME DESTINATION bin)
