project(uniof) # macros to help in unified treatment of different OF versions


add_library(uniof INTERFACE)

target_include_directories(uniof
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
  )
