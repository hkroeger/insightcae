project(test_toolkit)

add_executable(test_simplelatex test_simplelatex.cpp)
linkToolkitVtk(test_simplelatex Offscreen)
add_test(NAME test_toolkit_simplelatex
    COMMAND test_simplelatex
) 

add_subdirectory(analysis_parameterstudy)
