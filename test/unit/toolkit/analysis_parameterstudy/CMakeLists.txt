
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_library(simple_analysis SHARED simple_analysis.h simple_analysis.cpp)
target_link_libraries(simple_analysis toolkit)
target_include_directories(simple_analysis
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
target_add_PDL(simple_analysis)

add_test(NAME unit_simple_analysis_single COMMAND analyze --libs ${CMAKE_CURRENT_BINARY_DIR}/libsimple_analysis.so -x ${CMAKE_CURRENT_SOURCE_DIR}/simple_analysis.ist)
add_test(NAME unit_simple_analysis_parameterstudy COMMAND analyze --libs ${CMAKE_CURRENT_BINARY_DIR}/libsimple_analysis.so -x ${CMAKE_CURRENT_SOURCE_DIR}/simple_parameterstudy.ist) 
