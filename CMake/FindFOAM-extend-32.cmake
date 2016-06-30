# Try to find OpenFOAM-1.6-ext
# Once done this will define
#
# Fx32_FOUND          - system has foam-extend-3.2 installed

#FIND_PATH(Fx32_DIR NAMES etc/bashrc
FIND_FILE(Fx32_BASHRC NAMES bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/foam-extend-3.2/etc
  $ENV{HOME}/foam/foam-extend-3.2/etc
  /opt/foam/foam-extend-3.2/etc
)
message(STATUS ${Fx32_BASHRC})

SET(Fx32_FOUND FALSE)
IF(Fx32_BASHRC)
  #set(Fx32_BASHRC "${Fx32_DIR}/etc/bashrc")
  GET_FILENAME_COMPONENT(Fx32_ETC_DIR ${Fx32_BASHRC} PATH)
  GET_FILENAME_COMPONENT(Fx32_DIR ${Fx32_ETC_DIR} PATH)

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-c++FLAGS OUTPUT_VARIABLE Fx32_CXX_FLAGS)
  set(Fx32_CXX_FLAGS "${Fx32_CXX_FLAGS} -DFx32 -DOF16ext")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-WM_OPTIONS OUTPUT_VARIABLE Fx32_WM_OPTIONS)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-METIS_LIB_DIR OUTPUT_VARIABLE Fx32_METIS_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-MESQUITE_LIB_DIR OUTPUT_VARIABLE Fx32_MESQUITE_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-PARMETIS_LIB_DIR OUTPUT_VARIABLE Fx32_PARMETIS_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-SCOTCH_LIB_DIR OUTPUT_VARIABLE Fx32_SCOTCH_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-FOAM_APPBIN OUTPUT_VARIABLE Fx32_FOAM_APPBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-FOAM_LIBBIN OUTPUT_VARIABLE Fx32_FOAM_LIBBIN)

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFLibs ${Fx32_BASHRC} OUTPUT_VARIABLE Fx32_LIBRARIES)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFincPath ${Fx32_BASHRC} OUTPUT_VARIABLE Fx32_INCLUDE_PATHS)

  set(Fx32_LIBSRC_DIR "${Fx32_DIR}/src")
  set(Fx32_LIB_DIR "${Fx32_DIR}/lib/${Fx32_WM_OPTIONS}")
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-LINKLIBSO OUTPUT_VARIABLE Fx32_LINKLIBSO_full)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-LINKEXE OUTPUT_VARIABLE Fx32_LINKEXE_full)
  string(REGEX REPLACE "^[^ ]+" "" Fx32_LINKLIBSO ${Fx32_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" Fx32_LINKEXE ${Fx32_LINKEXE_full})
  message(STATUS "libso link flags = "  ${Fx32_LINKLIBSO})
  message(STATUS "exe link flags = "  ${Fx32_LINKEXE})
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx32_BASHRC} print-FOAM_MPI_LIBBIN OUTPUT_VARIABLE Fx32_FOAM_MPI_LIBBIN)

  set(Fx32_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/foam-extend-3.2")
  set(Fx32_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/foam-extend-3.2")

  list(APPEND INSIGHT_OFES_VARCONTENT "FX32@`find \\\${PATH//:/ } -maxdepth 1 -name insight.bashrc.fx32 -print -quit`#162")
  set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
alias fx32=\"source insight.bashrc.fx32\"
")
  create_script("insight.bashrc.fx32"
"source ${Fx32_BASHRC}

foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${Fx32_INSIGHT_LIB}
#- Clean LD_LIBRARY_PATH
cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
export PATH=$PATH:${Fx32_INSIGHT_BIN}
#- Clean PATH
cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
")

  macro (setup_exe_target_Fx32 targetname sources exename includes)
    add_executable(${targetname} ${sources})
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx32_INCLUDE_PATHS}")
    #LIST(APPEND allincludes "${Fx32_LIBSRC_DIR}/foam/lnInclude")
    #set_property(TARGET ${targetname} PROPERTY INCLUDE_DIRECTORIES ${allincludes})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx32_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx32_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${Fx32_INSIGHT_BIN})
    target_link_libraries(${targetname}
      ${Fx32_LIBRARIES} 
      #${Fx32_LIB_DIR}/libfoam.so 
      ${Fx32_FOAM_MPI_LIBBIN}/libPstream.so 
      #${Fx32_METIS_LIB_DIR}/libmetis.a
      ${Fx32_PARMETIS_LIB_DIR}/libparmetis.a
      ${Fx32_SCOTCH_LIB_DIR}/libscotch.so
      ${Fx32_SCOTCH_LIB_DIR}/libscotcherr.so
      ${Fx32_MESQUITE_LIB_DIR}/libmesquite.so
      ${ARGN})
     install(TARGETS ${targetname} RUNTIME DESTINATION ${Fx32_FOAM_APPBIN})
  endmacro()
  
  macro (setup_lib_target_Fx32 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    SET(LIB_SEARCHFLAGS "-L${Fx32_LIB_DIR} -L${Fx32_FOAM_MPI_LIBBIN} -L${Fx32_METIS_LIB_DIR} -L${Fx32_PARMETIS_LIB_DIR} -L${Fx32_SCOTCH_LIB_DIR} -L${Fx32_MESQUITE_LIB_DIR}")
    add_library(${targetname} SHARED ${sources})
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx32_INCLUDE_PATHS}")
    #LIST(APPEND allincludes "${Fx32_LIBSRC_DIR}/foam/lnInclude")
#    set_property(TARGET ${targetname} PROPERTY INCLUDE_DIRECTORIES ${allincludes})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx32_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx32_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${Fx32_INSIGHT_LIB})
    target_link_libraries(${targetname} ${ARGN}) 
    install(TARGETS ${targetname} LIBRARY DESTINATION ${Fx32_FOAM_LIBBIN})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(Fx32_FOUND TRUE)
ENDIF(Fx32_BASHRC)
