# Try to find OpenFOAM-2.2_engysEdition
# Once done this will define
#
# OF22eng_FOUND          - system has OpenFOAM-2.2_engysEdition installed



FIND_PATH(OF22eng_DIR NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2_engysEdition
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2_engysEdition-beta
  /opt/OpenFOAM/OpenFOAM-2.2_engysEdition
  /opt/OpenFOAM/OpenFOAM-2.2_engysEdition-beta
)
message(STATUS ${OF22eng_DIR})

SET(OF22eng_FOUND FALSE)

IF(OF22eng_DIR)
  set(OF22eng_BASHRC "${OF22eng_DIR}/etc/bashrc")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-c++FLAGS OUTPUT_VARIABLE OF22eng_CXX_FLAGS)
  set(OF22eng_CXX_FLAGS "${OF22eng_CXX_FLAGS} -DOF22eng")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-WM_OPTIONS OUTPUT_VARIABLE OF22eng_WM_OPTIONS)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-FOAM_EXT_LIBBIN OUTPUT_VARIABLE OF22eng_FOAM_EXT_LIBBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-SCOTCH_ROOT OUTPUT_VARIABLE OF22eng_SCOTCH_ROOT)

  set(OF22eng_LIBSRC_DIR "${OF22eng_DIR}/src")
  set(OF22eng_LIB_DIR "${OF22eng_DIR}/platforms/${OF22eng_WM_OPTIONS}/lib")
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-LINKLIBSO OUTPUT_VARIABLE OF22eng_LINKLIBSO_full)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-LINKEXE OUTPUT_VARIABLE OF22eng_LINKEXE_full)
  string(REGEX REPLACE "^[^ ]+" "" OF22eng_LINKLIBSO ${OF22eng_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF22eng_LINKEXE ${OF22eng_LINKEXE_full})
  message(STATUS "libso link flags = "  ${OF22eng_LINKLIBSO})
  message(STATUS "exe link flags = "  ${OF22eng_LINKEXE})
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-FOAM_MPI OUTPUT_VARIABLE OF22eng_MPI)

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-FOAM_APPBIN OUTPUT_VARIABLE OF22eng_FOAM_APPBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22eng_BASHRC} print-FOAM_LIBBIN OUTPUT_VARIABLE OF22eng_FOAM_LIBBIN)

  macro (setup_exe_target_OF22eng targetname sources exename includes)
    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    get_directory_property(temp LINK_DIRECTORIES)
    
    #link_directories(${OF22eng_LIB_DIR} ${OF22eng_LIB_DIR}/${OF22eng_MPI} ${OF22eng_FOAM_EXT_LIBBIN} "${OF22eng_SCOTCH_ROOT}/lib")
    #SET(LIB_SEARCHFLAGS "-L${OF22eng_LIB_DIR} -L${OF22eng_LIB_DIR}/${OF22eng_MPI} -L${OF22eng_FOAM_EXT_LIBBIN} -L${OF22eng_SCOTCH_ROOT}/lib")
    
    add_executable(${targetname} ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF22eng_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF22eng_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/OpenFOAM-2.2_engysEdition-beta)
    target_link_libraries(${targetname} 
      ${OF22eng_LIB_DIR}/libOpenFOAM.so 
      ${OF22eng_LIB_DIR}/${OF22eng_MPI}/libPstream.so 
      ${ARGN} ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF22eng_FOAM_APPBIN})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  macro (setup_lib_target_OF22eng targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    #link_directories(${OF22eng_LIB_DIR} ${OF22eng_LIB_DIR}/${OF22eng_MPI} ${OF22eng_FOAM_EXT_LIBBIN} "${OF22eng_SCOTCH_ROOT}/lib")
    SET(LIB_SEARCHFLAGS "-L${OF22eng_LIB_DIR} -L${OF22eng_LIB_DIR}/${OF22eng_MPI} -L${OF22eng_FOAM_EXT_LIBBIN} -L${OF22eng_SCOTCH_ROOT}/lib")
    add_library(${targetname} SHARED ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF22eng_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF22eng_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/OpenFOAM-2.2_engysEdition-beta)
    target_link_libraries(${targetname} ${ARGN}) 
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF22eng_FOAM_LIBBIN})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(OF22eng_FOUND TRUE)
ENDIF(OF22eng_DIR)

