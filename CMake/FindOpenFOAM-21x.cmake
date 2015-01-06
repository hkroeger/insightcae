# Try to find OpenFOAM-2.1.x
# Once done this will define
#
# OF21x_FOUND          - system has OpenFOAM-2.1.x installed



FIND_PATH(OF21x_DIR NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.1.x
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.1.0
  /opt/OpenFOAM/OpenFOAM-2.1.x
  /opt/OpenFOAM/OpenFOAM-2.1.0
)
message(STATUS ${OF21x_DIR})

SET(OF21x_FOUND FALSE)

IF(OF21x_DIR)
  set(OF21x_BASHRC "${OF21x_DIR}/etc/bashrc")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-WM_PROJECT_VERSION OUTPUT_VARIABLE OF21x_WM_PROJECT_VERSION)

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-c++FLAGS OUTPUT_VARIABLE OF21x_CXX_FLAGS)
  set(OF21x_CXX_FLAGS "${OF21x_CXX_FLAGS} -DOF21x")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-WM_OPTIONS OUTPUT_VARIABLE OF21x_WM_OPTIONS)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-FOAM_EXT_LIBBIN OUTPUT_VARIABLE OF21x_FOAM_EXT_LIBBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-SCOTCH_ROOT OUTPUT_VARIABLE OF21x_SCOTCH_ROOT)

  set(OF21x_LIBSRC_DIR "${OF21x_DIR}/src")
  set(OF21x_LIB_DIR "${OF21x_DIR}/platforms/${OF21x_WM_OPTIONS}/lib")
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-LINKLIBSO OUTPUT_VARIABLE OF21x_LINKLIBSO_full)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-LINKEXE OUTPUT_VARIABLE OF21x_LINKEXE_full)
  string(REGEX REPLACE "^[^ ]+" "" OF21x_LINKLIBSO ${OF21x_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF21x_LINKEXE ${OF21x_LINKEXE_full})
  message(STATUS "libso link flags = "  ${OF21x_LINKLIBSO})
  message(STATUS "exe link flags = "  ${OF21x_LINKEXE})
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-FOAM_MPI OUTPUT_VARIABLE OF21x_MPI)
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-FOAM_APPBIN OUTPUT_VARIABLE OF21x_FOAM_APPBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF21x_BASHRC} print-FOAM_LIBBIN OUTPUT_VARIABLE OF21x_FOAM_LIBBIN)

  macro (setup_exe_target_OF21x targetname sources exename includes)
    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    get_directory_property(temp LINK_DIRECTORIES)
    
    #link_directories(${OF21x_LIB_DIR} ${OF21x_LIB_DIR}/${OF21x_MPI} ${OF21x_FOAM_EXT_LIBBIN} "${OF21x_SCOTCH_ROOT}/lib")
    #SET(LIB_SEARCHFLAGS "-L${OF21x_LIB_DIR} -L${OF21x_LIB_DIR}/${OF21x_MPI} -L${OF21x_FOAM_EXT_LIBBIN} -L${OF21x_SCOTCH_ROOT}/lib")
    
    add_executable(${targetname} ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF21x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF21x_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OF21x_WM_PROJECT_VERSION})
    target_link_libraries(${targetname} 
      ${OF21x_LIB_DIR}/libOpenFOAM.so 
      ${OF21x_LIB_DIR}/${OF21x_MPI}/libPstream.so 
      ${ARGN} ) 

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF21x_FOAM_APPBIN})
  endmacro()
  
  macro (setup_lib_target_OF21x targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    #link_directories(${OF21x_LIB_DIR} ${OF21x_LIB_DIR}/${OF21x_MPI} ${OF21x_FOAM_EXT_LIBBIN} "${OF21x_SCOTCH_ROOT}/lib")
    SET(LIB_SEARCHFLAGS "-L${OF21x_LIB_DIR} -L${OF21x_LIB_DIR}/${OF21x_MPI} -L${OF21x_FOAM_EXT_LIBBIN} -L${OF21x_SCOTCH_ROOT}/lib")
    add_library(${targetname} SHARED ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF21x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF21x_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OF21x_WM_PROJECT_VERSION})
    target_link_libraries(${targetname} ${ARGN}) 
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF21x_FOAM_LIBBIN})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(OF21x_FOUND TRUE)
ENDIF(OF21x_DIR)

