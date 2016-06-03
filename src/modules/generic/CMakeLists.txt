project(genericmodules)

#find_package(OpenCasCade REQUIRED)

set(genericmodules_SOURCES 
  filetemplate.cpp 
  )
  
file(GLOB_RECURSE genericmodules_HEADERS "*.h")
include_directories(${toolkit_SOURCE_DIR})
include_directories(${refdata_SOURCE_DIR})
include_directories(${GSL_INCLUDE_DIR})
#include_directories(${OCC_INCLUDE_DIR})
#include_directories(${journalbearingcommon_SOURCE_DIR})

add_library(genericmodules SHARED ${genericmodules_SOURCES})

add_PDL(genericmodules "${genericmodules_HEADERS}")
set(genericmodules_LIBS
  toolkit
  refdata
  ${GSL_LIBRARIES}
)
target_link_libraries(genericmodules ${genericmodules_LIBS})
target_include_directories(genericmodules
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
install(TARGETS genericmodules LIBRARY DESTINATION lib)

# add_pybindings(GenericModules "genericmodules.i" genericmodules)
