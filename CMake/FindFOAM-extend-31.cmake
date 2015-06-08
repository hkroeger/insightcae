# Try to find OpenFOAM-1.6-ext
# Once done this will define
#
# Fx31_FOUND          - system has foam-extend-3.1 installed

#FIND_PATH(Fx31_DIR NAMES etc/bashrc
FIND_FILE(Fx31_BASHRC NAMES bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/foam-extend-3.1/etc
  $ENV{HOME}/foam/foam-extend-3.1/etc
  /opt/foam/foam-extend-3.1/etc
)
message(STATUS ${Fx31_BASHRC})

SET(Fx31_FOUND FALSE)
IF(Fx31_BASHRC)
  #set(Fx31_BASHRC "${Fx31_DIR}/etc/bashrc")
  GET_FILENAME_COMPONENT(Fx31_ETC_DIR ${Fx31_BASHRC} PATH)
  GET_FILENAME_COMPONENT(Fx31_DIR ${Fx31_ETC_DIR} PATH)

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-c++FLAGS OUTPUT_VARIABLE Fx31_CXX_FLAGS)
  set(Fx31_CXX_FLAGS "${Fx31_CXX_FLAGS} -DFx31 -DOF16ext")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-WM_OPTIONS OUTPUT_VARIABLE Fx31_WM_OPTIONS)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-METIS_LIB_DIR OUTPUT_VARIABLE Fx31_METIS_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-MESQUITE_LIB_DIR OUTPUT_VARIABLE Fx31_MESQUITE_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-PARMETIS_LIB_DIR OUTPUT_VARIABLE Fx31_PARMETIS_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-SCOTCH_LIB_DIR OUTPUT_VARIABLE Fx31_SCOTCH_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-FOAM_APPBIN OUTPUT_VARIABLE Fx31_FOAM_APPBIN)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-FOAM_LIBBIN OUTPUT_VARIABLE Fx31_FOAM_LIBBIN)

  set(Fx31_LIBSRC_DIR "${Fx31_DIR}/src")
  set(Fx31_LIB_DIR "${Fx31_DIR}/lib/${Fx31_WM_OPTIONS}")
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-LINKLIBSO OUTPUT_VARIABLE Fx31_LINKLIBSO_full)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-LINKEXE OUTPUT_VARIABLE Fx31_LINKEXE_full)
  string(REGEX REPLACE "^[^ ]+" "" Fx31_LINKLIBSO ${Fx31_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" Fx31_LINKEXE ${Fx31_LINKEXE_full})
  message(STATUS "libso link flags = "  ${Fx31_LINKLIBSO})
  message(STATUS "exe link flags = "  ${Fx31_LINKEXE})
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${Fx31_BASHRC} print-FOAM_MPI_LIBBIN OUTPUT_VARIABLE Fx31_FOAM_MPI_LIBBIN)

  macro (setup_exe_target_Fx31 targetname sources exename includes)
    add_executable(${targetname} ${sources})
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx31_LIBSRC_DIR}/foam/lnInclude")
    set_property(TARGET ${targetname} PROPERTY INCLUDE_DIRECTORIES ${allincludes})
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx31_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx31_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/foam-extend-3.1)
    target_link_libraries(${targetname} 
      ${Fx31_LIB_DIR}/libfoam.so 
      ${Fx31_FOAM_MPI_LIBBIN}/libPstream.so 
      #${Fx31_METIS_LIB_DIR}/libmetis.a
      ${Fx31_PARMETIS_LIB_DIR}/libparmetis.a
      ${Fx31_SCOTCH_LIB_DIR}/libscotch.so
      ${Fx31_SCOTCH_LIB_DIR}/libscotcherr.so
      ${Fx31_MESQUITE_LIB_DIR}/libmesquite.so
      ${ARGN})
     install(TARGETS ${targetname} RUNTIME DESTINATION ${Fx31_FOAM_APPBIN})
  endmacro()
  
  macro (setup_lib_target_Fx31 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    SET(LIB_SEARCHFLAGS "-L${Fx31_LIB_DIR} -L${Fx31_FOAM_MPI_LIBBIN} -L${Fx31_METIS_LIB_DIR} -L${Fx31_PARMETIS_LIB_DIR} -L${Fx31_SCOTCH_LIB_DIR} -L${Fx31_MESQUITE_LIB_DIR}")
    add_library(${targetname} SHARED ${sources})
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx31_LIBSRC_DIR}/foam/lnInclude")
    set_property(TARGET ${targetname} PROPERTY INCLUDE_DIRECTORIES ${allincludes})
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx31_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx31_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/foam-extend-3.1)
    target_link_libraries(${targetname} ${ARGN}) 
    install(TARGETS ${targetname} LIBRARY DESTINATION ${Fx31_FOAM_LIBBIN})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(Fx31_FOUND TRUE)
ENDIF(Fx31_BASHRC)
