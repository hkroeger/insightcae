project(genericmodules)

#find_package(OpenCasCade REQUIRED)

set(genericmodules_SOURCES 
  filetemplate.cpp 
  convergenceanalysis.cpp
)

set(genericmodules_LIBS
  toolkit
  refdata
  ${GSL_LIBRARIES}
)

if (INSIGHT_BUILD_OPENFOAM AND INSIGHT_BUILD_CAD)
    LIST(APPEND genericmodules_SOURCES
        numericalwindtunnel.cpp
        internalpressureloss.cpp
    )
    LIST(APPEND genericmodules_LIBS insightcad)
endif()

file(GLOB_RECURSE genericmodules_HEADERS "*.h")
include_directories(${toolkit_SOURCE_DIR})
include_directories(${refdata_SOURCE_DIR})
include_directories(${GSL_INCLUDE_DIR})
#include_directories(${OCC_INCLUDE_DIR})
#include_directories(${journalbearingcommon_SOURCE_DIR})

add_library(genericmodules SHARED ${genericmodules_SOURCES})

add_PDL(genericmodules "${genericmodules_HEADERS}")
target_link_libraries(genericmodules ${genericmodules_LIBS})
target_include_directories(genericmodules
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
install(TARGETS genericmodules LIBRARY DESTINATION lib)

install_shared_file(genericmodules_module_cfg genericmodules.module modules.d)

add_pybindings(GenericModules "genericmodules.i" genericmodules)
