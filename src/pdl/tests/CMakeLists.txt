project(testPDL)

file(GLOB_RECURSE test_pdl_HEADERS "*.h")
add_executable(test_pdl test_pdl.cpp)
linkToolkitVtk(test_pdl Offscreen)
target_include_directories(test_pdl
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  )
add_PDL(test_pdl "${test_pdl_HEADERS}")

#add_test(NAME test_pdl COMMAND test_pdl)
