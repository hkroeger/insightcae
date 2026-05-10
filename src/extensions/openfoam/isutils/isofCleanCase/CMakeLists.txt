project(isofCleanCase)


add_executable(isofCleanCase isofcleancase.cpp)
target_enable_qt(isofCleanCase)
target_link_libraries(isofCleanCase Qt5::Widgets toolkit_gui)
linkToolkitVtk(isofCleanCase Onscreen)
install(TARGETS isofCleanCase RUNTIME DESTINATION bin)
