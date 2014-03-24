# Try to find OpenFOAM-2.2.x
# Once done this will define
#
# OF22x_FOUND          - system has OpenFOAM-2.2.x installed



FIND_PATH(OF22x_DIR NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2.x
  /opt/OpenFOAM/OpenFOAM-2.2.x
)
message(STATUS ${OF22x_DIR})

SET(OF22x_FOUND FALSE)

IF(OF22x_DIR)
  set(OF22x_BASHRC "${OF22x_DIR}/etc/bashrc")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-c++FLAGS OUTPUT_VARIABLE OF22x_CXX_FLAGS)
  set(OF22x_CXX_FLAGS "${OF22x_CXX_FLAGS} -DOF22x")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-WM_OPTIONS OUTPUT_VARIABLE OF22x_WM_OPTIONS)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-FOAM_EXT_LIBBIN OUTPUT_VARIABLE OF22x_FOAM_EXT_LIBBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-SCOTCH_ROOT OUTPUT_VARIABLE OF22x_SCOTCH_ROOT)

  set(OF22x_LIBSRC_DIR "${OF22x_DIR}/src")
  set(OF22x_LIB_DIR "${OF22x_DIR}/platforms/${OF22x_WM_OPTIONS}/lib")
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-LINKLIBSO OUTPUT_VARIABLE OF22x_LINKLIBSO_full)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-LINKEXE OUTPUT_VARIABLE OF22x_LINKEXE_full)
  string(REGEX REPLACE "^[^ ]+" "" OF22x_LINKLIBSO ${OF22x_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF22x_LINKEXE ${OF22x_LINKEXE_full})
  message(STATUS "libso link flags = "  ${OF22x_LINKLIBSO})
  message(STATUS "exe link flags = "  ${OF22x_LINKEXE})
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-FOAM_MPI OUTPUT_VARIABLE OF22x_MPI)

  macro (setup_exe_target_OF22x targetname sources exename includes)
    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    get_directory_property(temp LINK_DIRECTORIES)
    
    #link_directories(${OF22x_LIB_DIR} ${OF22x_LIB_DIR}/${OF22x_MPI} ${OF22x_FOAM_EXT_LIBBIN} "${OF22x_SCOTCH_ROOT}/lib")
    #SET(LIB_SEARCHFLAGS "-L${OF22x_LIB_DIR} -L${OF22x_LIB_DIR}/${OF22x_MPI} -L${OF22x_FOAM_EXT_LIBBIN} -L${OF22x_SCOTCH_ROOT}/lib")
    
    add_executable(${targetname} ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF22x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF22x_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/OpenFOAM-2.2.x)
    target_link_libraries(${targetname} 
      ${OF22x_LIB_DIR}/libOpenFOAM.so 
      ${OF22x_LIB_DIR}/${OF22x_MPI}/libPstream.so 
      ${ARGN} ) 

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  macro (setup_lib_target_OF22x targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    #link_directories(${OF22x_LIB_DIR} ${OF22x_LIB_DIR}/${OF22x_MPI} ${OF22x_FOAM_EXT_LIBBIN} "${OF22x_SCOTCH_ROOT}/lib")
    SET(LIB_SEARCHFLAGS "-L${OF22x_LIB_DIR} -L${OF22x_LIB_DIR}/${OF22x_MPI} -L${OF22x_FOAM_EXT_LIBBIN} -L${OF22x_SCOTCH_ROOT}/lib")
    add_library(${targetname} SHARED ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF22x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF22x_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/OpenFOAM-2.2.x)
    target_link_libraries(${targetname} ${ARGN}) 
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(OF22x_FOUND TRUE)
ENDIF(OF22x_DIR)

