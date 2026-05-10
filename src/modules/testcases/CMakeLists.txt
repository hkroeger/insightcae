project(testcases)


set(testcases_LIBS
  toolkit toolkit_cad
)
if (PYTHONLIBS_FOUND AND INSIGHT_BUILD_PYTHONBINDINGS) # required for refdata
    list(APPEND testcases_SOURCES
        #decayingturbulence.cpp
        pipe.cpp pipe.h
        channel.cpp channel.h
        #ercoftac_squaresection180degreebend.cpp
        #freeshearflow.cpp
        )
    list(APPEND testcases_LIBS
        refdata
        )
    ADD_DEFINITIONS( -DHAS_REFDATA )
endif()


add_library(testcases SHARED
  airfoilsection.cpp airfoilsection.h
  flatplatebl.cpp flatplatebl.h
  ${testcases_SOURCES}
)
target_add_PDL(testcases)

target_link_libraries(testcases ${testcases_LIBS})
target_include_directories(testcases
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
install(TARGETS testcases LIBRARY DESTINATION lib)

add_shared_file(testcases modules.d testcases.module)
add_shared_file(testcases testcases
  airfoilsection_sketches_1.png
  airfoilsection_sketches_L.png
  airfoilsection_sketches_H.png
  airfoilsection_sketches_alpha.png
  airfoilsection_sketches_csv.png
)

if (PYTHONLIBS_FOUND AND INSIGHT_BUILD_PYTHONBINDINGS)

add_pybindings(TestcaseModules "testcasemodules.i" testcases)

install_script(isCreateChannelProfiles isCreateChannelProfiles.py)

endif()



add_library(testcasesgui SHARED
  testcases_gui.cpp
  testcases.qrc
)
target_enable_qt(testcasesgui)
target_link_libraries(testcasesgui testcases toolkit_gui)

install(TARGETS testcasesgui LIBRARY DESTINATION lib COMPONENT ${INSIGHT_INSTALL_COMPONENT})
