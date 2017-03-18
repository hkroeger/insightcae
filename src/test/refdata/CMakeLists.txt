project(test_refdata)

add_executable(test_refdata test_refdata.cpp)
target_link_libraries(test_refdata refdata ${PYTHON_LIBRARIES})
add_test(NAME test_refdata COMMAND test_refdata) 

