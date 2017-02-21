project(test_toolkit)

add_executable(test_simplelatex test_simplelatex.cpp)
target_link_libraries(test_simplelatex toolkit)
add_test(NAME test_toolkit_simplelatex
    COMMAND test_simplelatex
) 

add_subdirectory(analysis_parameterstudy)
