# Try to find OpenFOAM-2.2_engysEdition
# Once done this will define
#
# OF22eng_FOUND          - system has OpenFOAM-2.2_engysEdition installed



#FIND_PATH(OF22eng_DIR NAMES etc/bashrc
FIND_FILE(OF22eng_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2_engysEdition
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2_engysEdition-beta
  /opt/OpenFOAM/OpenFOAM-2.2_engysEdition
  /opt/OpenFOAM/OpenFOAM-2.2_engysEdition-beta
)
message(STATUS ${OF22eng_BASHRC})

SET(OF22eng_FOUND FALSE)

IF(OF22eng_BASHRC)
  #set(OF22eng_BASHRC "${OF22eng_DIR}/etc/bashrc")
  GET_FILENAME_COMPONENT(OF22eng_ETC_DIR ${OF22eng_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OF22eng_DIR ${OF22eng_ETC_DIR} PATH)

  detectEnvVars(OF22eng
    WM_PROJECT WM_PROJECT_VERSION WM_OPTIONS
    FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN
    c++FLAGS
    LINKLIBSO LINKEXE
    FOAM_MPI
  )

  filterWarningFlags(OF22eng_c++FLAGS)
  set(OF22eng_CXX_FLAGS "${OF22eng_c++FLAGS} -DOF22eng -DOF_VERSION=020200 -DOF_FORK_engys")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFLibs ${OF22eng_BASHRC} OUTPUT_VARIABLE OF22eng_LIBRARIES)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFincPath ${OF22eng_BASHRC} OUTPUT_VARIABLE OF22eng_INCLUDE_PATHS)

  set(OF22eng_LIBSRC_DIR "${OF22eng_DIR}/src")
  set(OF22eng_LIB_DIR "${OF22eng_DIR}/platforms/${OF22eng_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" OF22eng_LINKLIBSO ${OF22eng_LINKLIBSO})
  string(REGEX REPLACE "^[^ ]+" "" OF22eng_LINKEXE ${OF22eng_LINKEXE})
  message(STATUS "libso link flags = "  ${OF22eng_LINKLIBSO})
  message(STATUS "exe link flags = "  ${OF22eng_LINKEXE})


  set(OF22eng_INSIGHT_INSTALL_BIN "bin/${OF22eng_WM_PROJECT}-${OF22eng_WM_PROJECT_VERSION}")
  set(OF22eng_INSIGHT_INSTALL_LIB "lib/${OF22eng_WM_PROJECT}-${OF22eng_WM_PROJECT_VERSION}")
  set(OF22eng_INSIGHT_BIN "${CMAKE_BINARY_DIR}/${OF22eng_INSIGHT_INSTALL_BIN}")
  set(OF22eng_INSIGHT_LIB "${CMAKE_BINARY_DIR}/${OF22eng_INSIGHT_INSTALL_LIB}")

  addOFConfig(OF22eng of22eng 220)

  macro (setup_exe_target_OF22eng targetname sources exename includes)
    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    get_directory_property(temp LINK_DIRECTORIES)
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF22eng_INCLUDE_PATHS}")

    
    add_executable(${targetname} ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF22eng_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF22eng_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OF22eng_INSIGHT_BIN})
    set_target_properties(${targetname} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    target_link_libraries(${targetname}
      ${OF22eng_LIBRARIES}
      #${OF22eng_LIB_DIR}/libOpenFOAM.so 
      ${OF22eng_LIB_DIR}/${OF22eng_FOAM_MPI}/libPstream.so
      ${ARGN} ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF22eng_INSIGHT_INSTALL_BIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  macro (setup_lib_target_OF22eng targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF22eng_INCLUDE_PATHS}")

    SET(LIB_SEARCHFLAGS "-L${OF22eng_LIB_DIR} -L${OF22eng_LIB_DIR}/${OF22eng_FOAM_MPI} -L${OF22eng_FOAM_EXT_LIBBIN} -L${OF22eng_SCOTCH_ROOT}/lib")
    add_library(${targetname} SHARED ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF22eng_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF22eng_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OF22eng_INSIGHT_LIB})
    set_target_properties(${targetname} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    target_link_libraries(${targetname} ${OF22eng_LIBRARIES} ${ARGN})
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF22eng_INSIGHT_INSTALL_LIB} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(OF22eng_FOUND TRUE)
ENDIF(OF22eng_BASHRC)

