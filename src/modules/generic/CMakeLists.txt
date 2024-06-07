project(genericmodules)

#find_package(OpenCasCade REQUIRED)

set(genericmodules_SOURCES 
  filetemplate.cpp 
  convergenceanalysis.cpp
)

set(genericmodules_LIBS
  toolkit insightcad
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

set(genericmodulesgui_SOURCES
    numericalwindtunnel_gui.cpp
    internalpressureloss_gui.cpp
)
add_library(genericmodulesgui SHARED ${genericmodulesgui_SOURCES})
target_link_libraries(genericmodulesgui genericmodules toolkit_gui)

install(TARGETS genericmodulesgui LIBRARY DESTINATION lib COMPONENT ${INSIGHT_INSTALL_COMPONENT})


add_pybindings(GenericModules "genericmodules.i" genericmodules)
