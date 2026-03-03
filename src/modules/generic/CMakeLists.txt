project(genericmodules)



add_library(genericmodules SHARED
    filetemplate.cpp filetemplate.h
    convergenceanalysis.cpp convergenceanalysis.h
    numericalwindtunnel.cpp numericalwindtunnel.h numericalwindtunnel_eval.cpp
    internalpressureloss.cpp internalpressureloss.h
    internalpressureloss_supplementedinputdata.cpp
    numericalwindtunnel_supplementedinputdata.cpp
)
target_add_PDL(genericmodules)
target_link_libraries(genericmodules toolkit toolkit_cad)
target_include_directories(genericmodules
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
if (NOT WIN32)
    install(TARGETS genericmodules LIBRARY DESTINATION lib)
else()
    install(TARGETS genericmodules ARCHIVE DESTINATION lib)
endif()

add_shared_file(genericmodules modules.d genericmodules.module)



add_library(genericmodulesgui SHARED
    numericalwindtunnel_gui.h numericalwindtunnel_gui.cpp
    internalpressureloss_gui.cpp
    genericmodules.qrc
)
target_enable_qt(genericmodulesgui)
target_link_libraries(genericmodulesgui genericmodules toolkit_gui)

install(TARGETS genericmodulesgui LIBRARY DESTINATION lib COMPONENT ${INSIGHT_INSTALL_COMPONENT})


add_pybindings(GenericModules "genericmodules.i" genericmodules)

target_sources(GenericModules_python
  PRIVATE
    numericalwindtunnel_gui.h
)
