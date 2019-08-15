project(uniof) # macros to help in unified treatment of different OF versions

# set(uniof_SOURCES uniof.cpp)

add_library(uniof INTERFACE)
# target_link_libraries(uniof )

target_include_directories(uniof
#   INTERFACE ${CMAKE_CURRENT_BINARY_DIR} 
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
  )
# target_sources(uniof INTERFACE 
#   ${CMAKE_CURRENT_SOURCE_DIR}/uniof.h
# )
# install(TARGETS uniof LIBRARY DESTINATION lib)
