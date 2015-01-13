project(pdl)

set(pdl_SOURCES pdl.cpp)

add_executable(pdl ${pdl_SOURCES})
target_link_libraries(pdl ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})

install(TARGETS pdl RUNTIME DESTINATION bin)