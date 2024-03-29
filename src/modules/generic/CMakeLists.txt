project(genericmodules)

#find_package(OpenCasCade REQUIRED)

set(genericmodules_SOURCES 
  filetemplate.cpp 
  convergenceanalysis.cpp
)

set(genericmodules_LIBS
  toolkit
  toolkit_gui
)

if (INSIGHT_BUILD_OPENFOAM AND INSIGHT_BUILD_CAD)
    LIST(APPEND genericmodules_SOURCES
        numericalwindtunnel.cpp
        internalpressureloss.cpp
    )
    LIST(APPEND genericmodules_LIBS insightcad)
endif()

file(GLOB_RECURSE genericmodules_HEADERS "*.h")

add_library(genericmodules SHARED ${genericmodules_SOURCES})

add_PDL(genericmodules "${genericmodules_HEADERS}")
target_link_libraries(genericmodules ${genericmodules_LIBS})
target_include_directories(genericmodules
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
if (NOT WIN32)
    install(TARGETS genericmodules LIBRARY DESTINATION lib)
else()
    install(TARGETS genericmodules ARCHIVE DESTINATION lib)
endif()

install_shared_file(genericmodules_module_cfg genericmodules.module modules.d)

add_pybindings(GenericModules "genericmodules.i" genericmodules)
