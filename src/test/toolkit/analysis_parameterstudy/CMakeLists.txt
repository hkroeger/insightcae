
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

set(simpleanalysis_SOURCES simple_analysis.cpp )
add_library(simple_analysis SHARED ${simpleanalysis_SOURCES})
target_link_libraries(simple_analysis toolkit)
target_include_directories(simple_analysis
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
add_PDL(simple_analysis ${CMAKE_CURRENT_SOURCE_DIR}/${simpleanalysis_SOURCES})

add_test(NAME test_simple_analysis_single COMMAND analyze --libs ${CMAKE_CURRENT_BINARY_DIR}/libsimple_analysis.so -x ${CMAKE_CURRENT_SOURCE_DIR}/simple_analysis.ist)
add_test(NAME test_simple_analysis_parameterstudy COMMAND analyze --libs ${CMAKE_CURRENT_BINARY_DIR}/libsimple_analysis.so -x ${CMAKE_CURRENT_SOURCE_DIR}/simple_parameterstudy.ist) 
