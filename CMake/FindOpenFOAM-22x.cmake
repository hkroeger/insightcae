# Try to find OpenFOAM-2.2.x
# Once done this will define
#
# OF22x_FOUND          - system has OpenFOAM-2.2.x installed



#FIND_PATH(OF22x_DIR NAMES etc/bashrc
FIND_FILE(OF22x_BASHRC NAMES bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2.x/etc
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2.0/etc
  /opt/OpenFOAM/OpenFOAM-2.2.x/etc
  /opt/OpenFOAM/OpenFOAM-2.2.0/etc
)
message(STATUS ${OF22x_BASHRC})

SET(OF22x_FOUND FALSE)

IF(OF22x_BASHRC)
  #set(OF22x_BASHRC "${OF22x_DIR}/etc/bashrc")
  GET_FILENAME_COMPONENT(OF22x_ETC_DIR ${OF22x_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OF22x_DIR ${OF22x_ETC_DIR} PATH)
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-WM_PROJECT_VERSION OUTPUT_VARIABLE OF22x_WM_PROJECT_VERSION)

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

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-FOAM_APPBIN OUTPUT_VARIABLE OF22x_FOAM_APPBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF22x_BASHRC} print-FOAM_LIBBIN OUTPUT_VARIABLE OF22x_FOAM_LIBBIN)

  set(OF22x_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OF22x_WM_PROJECT_VERSION}")
  set(OF22x_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OF22x_WM_PROJECT_VERSION}")

  list(APPEND INSIGHT_OFES_VARCONTENT "OF22x@`find \\\${PATH//:/ } -maxdepth 1 -name insight.bashrc.of22x -print -quit`#220")
  set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
alias of22x=\"source insight.bashrc.of22x\"
")
  create_script("insight.bashrc.of22x"
"source ${OF22x_BASHRC}

foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${OF22x_INSIGHT_LIB}
#- Clean LD_LIBRARY_PATH
cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
export PATH=$PATH:${OF22x_INSIGHT_BIN}
#- Clean PATH
cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
")

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
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OF22x_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${OF22x_LIB_DIR}/libOpenFOAM.so 
      ${OF22x_LIB_DIR}/${OF22x_MPI}/libPstream.so 
      ${ARGN} ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF22x_FOAM_APPBIN})

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
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OF22x_INSIGHT_LIB})
    target_link_libraries(${targetname} ${ARGN}) 
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF22x_FOAM_LIBBIN})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(OF22x_FOUND TRUE)
ENDIF(OF22x_BASHRC)

