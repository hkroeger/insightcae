project(globalRegistry) # macros to help in unified treatment of different OF versions


add_library(globalRegistry INTERFACE)

target_include_directories(globalRegistry
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
  )
