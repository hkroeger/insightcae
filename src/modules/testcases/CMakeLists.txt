if (PYTHONLIBS_FOUND)

project(testcases)

#find_package(OpenCasCade REQUIRED)

set(testcases_SOURCES 
  airfoilsection.cpp
  flatplatebl.cpp
  )

set(testcases_LIBS
  toolkit
)
if (INSIGHT_BUILD_PYTHONBINDINGS) # required for refdata
    list(APPEND testcases_SOURCES
        #decayingturbulence.cpp
        pipe.cpp
        channel.cpp
        #ercoftac_squaresection180degreebend.cpp
        #freeshearflow.cpp
        )
    list(APPEND testcases_LIBS
        refdata
        )
    ADD_DEFINITIONS( -DHAS_REFDATA )
endif()
  
file(GLOB_RECURSE testcases_HEADERS "*.h")


add_library(testcases SHARED ${testcases_SOURCES})
add_PDL(testcases "${testcases_HEADERS}")

target_link_libraries(testcases ${testcases_LIBS})
target_include_directories(testcases
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  )
install(TARGETS testcases LIBRARY DESTINATION lib)

add_pybindings(TestcaseModules "testcasemodules.i" testcases)

install_shared_file(testcases_module_cfg testcases.module modules.d)
install_shared_file(doc_airfoilsection_sketches_1 airfoilsection_sketches_1.png testcases)
install_shared_file(doc_airfoilsection_sketches_L airfoilsection_sketches_L.png testcases)
install_shared_file(doc_airfoilsection_sketches_H airfoilsection_sketches_H.png testcases)
install_shared_file(doc_airfoilsection_sketches_alpha airfoilsection_sketches_alpha.png testcases)
install_shared_file(doc_airfoilsection_sketches_csv airfoilsection_sketches_csv.png testcases)

install_script(isCreateChannelProfiles isCreateChannelProfiles.py)

endif()
