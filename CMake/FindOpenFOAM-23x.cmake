# Try to find OpenFOAM-2.3.x
# Once done this will define
#
# OF23x_FOUND          - system has OpenFOAM-2.3.x installed



#FIND_PATH(OF23x_DIR NAMES etc/bashrc
FIND_FILE(OF23x_BASHRC NAMES bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.3.x/etc
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.3.0/etc
  /opt/OpenFOAM/OpenFOAM-2.3.x/etc
  /opt/OpenFOAM/OpenFOAM-2.3.0/etc
  /opt/openfoam230/etc
)

message(STATUS ${OF23x_BASHRC})

SET(OF23x_FOUND FALSE)

IF(OF23x_BASHRC)
  #set(OF23x_BASHRC "${OF23x_DIR}/etc/bashrc")
  GET_FILENAME_COMPONENT(OF23x_ETC_DIR ${OF23x_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OF23x_DIR ${OF23x_ETC_DIR} PATH)

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-WM_PROJECT_VERSION OUTPUT_VARIABLE OF23x_WM_PROJECT_VERSION)
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-c++FLAGS OUTPUT_VARIABLE OF23x_CXX_FLAGS)
  set(OF23x_CXX_FLAGS "${OF23x_CXX_FLAGS} -DOF23x")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-WM_OPTIONS OUTPUT_VARIABLE OF23x_WM_OPTIONS)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-FOAM_EXT_LIBBIN OUTPUT_VARIABLE OF23x_FOAM_EXT_LIBBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-SCOTCH_ROOT OUTPUT_VARIABLE OF23x_SCOTCH_ROOT)

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFLibs ${OF23x_BASHRC} OUTPUT_VARIABLE OF23x_LIBRARIES)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFincPath ${OF23x_BASHRC} OUTPUT_VARIABLE OF23x_INCLUDE_PATHS)

  set(OF23x_LIBSRC_DIR "${OF23x_DIR}/src")
  set(OF23x_LIB_DIR "${OF23x_DIR}/platforms/${OF23x_WM_OPTIONS}/lib")
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-LINKLIBSO OUTPUT_VARIABLE OF23x_LINKLIBSO_full)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-LINKEXE OUTPUT_VARIABLE OF23x_LINKEXE_full)
  string(REGEX REPLACE "^[^ ]+" "" OF23x_LINKLIBSO ${OF23x_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF23x_LINKEXE ${OF23x_LINKEXE_full})
  message(STATUS "libso link flags = "  ${OF23x_LINKLIBSO})
  message(STATUS "exe link flags = "  ${OF23x_LINKEXE})
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-FOAM_MPI OUTPUT_VARIABLE OF23x_MPI)

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-FOAM_APPBIN OUTPUT_VARIABLE OF23x_FOAM_APPBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF23x_BASHRC} print-FOAM_LIBBIN OUTPUT_VARIABLE OF23x_FOAM_LIBBIN)
  
  set(OF23x_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OF23x_WM_PROJECT_VERSION}")
  set(OF23x_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OF23x_WM_PROJECT_VERSION}")

  list(APPEND INSIGHT_OFES_VARCONTENT "OF23x@`find \\\${PATH//:/ } -maxdepth 1 -name insight.bashrc.of23x -print -quit`#230")
  set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
alias of23x=\"source insight.bashrc.of23x\"
")
  create_script("insight.bashrc.of23x"
"source ${OF23x_BASHRC}

foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${OF23x_INSIGHT_LIB}
#- Clean LD_LIBRARY_PATH
cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
export PATH=$PATH:${OF23x_INSIGHT_BIN}
#- Clean PATH
cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
")

  macro (setup_exe_target_OF23x targetname sources exename includes)
    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    get_directory_property(temp LINK_DIRECTORIES)
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF23x_INCLUDE_PATHS}")

    #link_directories(${OF23x_LIB_DIR} ${OF23x_LIB_DIR}/${OF23x_MPI} ${OF23x_FOAM_EXT_LIBBIN} "${OF23x_SCOTCH_ROOT}/lib")
    #SET(LIB_SEARCHFLAGS "-L${OF23x_LIB_DIR} -L${OF23x_LIB_DIR}/${OF23x_MPI} -L${OF23x_FOAM_EXT_LIBBIN} -L${OF23x_SCOTCH_ROOT}/lib")
    
    add_executable(${targetname} ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF23x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF23x_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OF23x_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${OF23x_LIBRARIES}
      #${OF23x_LIB_DIR}/libOpenFOAM.so 
      ${OF23x_LIB_DIR}/${OF23x_MPI}/libPstream.so 
      ${ARGN} ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF23x_FOAM_APPBIN})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  macro (setup_lib_target_OF23x targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF23x_INCLUDE_PATHS}")
#     message(STATUS "target " ${targetname} ": includes=" ${includes})
    #link_directories(${OF23x_LIB_DIR} ${OF23x_LIB_DIR}/${OF23x_MPI} ${OF23x_FOAM_EXT_LIBBIN} "${OF23x_SCOTCH_ROOT}/lib")
    SET(LIB_SEARCHFLAGS "-L${OF23x_LIB_DIR} -L${OF23x_LIB_DIR}/${OF23x_MPI} -L${OF23x_FOAM_EXT_LIBBIN} -L${OF23x_SCOTCH_ROOT}/lib")
    add_library(${targetname} SHARED ${sources})
    target_link_libraries(${targetname} ${ARGN}) 
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF23x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF23x_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OF23x_INSIGHT_LIB})
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF23x_FOAM_LIBBIN})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(OF23x_FOUND TRUE)
ENDIF(OF23x_BASHRC)

