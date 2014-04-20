# Try to find OpenFOAM-1.6-ext
# Once done this will define
#
# OF16ext_FOUND          - system has OpenFOAM-1.6-ext installed



FIND_PATH(OF16ext_DIR NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-1.6-ext
  /opt/OpenFOAM/OpenFOAM-1.6-ext
)
message(STATUS ${OF16ext_DIR})

SET(OF16ext_FOUND FALSE)
IF(OF16ext_DIR)
  set(OF16ext_BASHRC "${OF16ext_DIR}/etc/bashrc")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-c++FLAGS OUTPUT_VARIABLE OF16ext_CXX_FLAGS)
  set(OF16ext_CXX_FLAGS "${OF16ext_CXX_FLAGS} -DOF16ext")

  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-WM_OPTIONS OUTPUT_VARIABLE OF16ext_WM_OPTIONS)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-METIS_LIB_DIR OUTPUT_VARIABLE OF16ext_METIS_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-MESQUITE_LIB_DIR OUTPUT_VARIABLE OF16ext_MESQUITE_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-PARMETIS_LIB_DIR OUTPUT_VARIABLE OF16ext_PARMETIS_LIB_DIR)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-SCOTCH_LIB_DIR OUTPUT_VARIABLE OF16ext_SCOTCH_LIB_DIR)

  set(OF16ext_LIBSRC_DIR "${OF16ext_DIR}/src")
  set(OF16ext_LIB_DIR "${OF16ext_DIR}/lib/${OF16ext_WM_OPTIONS}")
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-LINKLIBSO OUTPUT_VARIABLE OF16ext_LINKLIBSO_full)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-LINKEXE OUTPUT_VARIABLE OF16ext_LINKEXE_full)
  string(REGEX REPLACE "^[^ ]+" "" OF16ext_LINKLIBSO ${OF16ext_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF16ext_LINKEXE ${OF16ext_LINKEXE_full})
  message(STATUS "libso link flags = "  ${OF16ext_LINKLIBSO})
  message(STATUS "exe link flags = "  ${OF16ext_LINKEXE})
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-FOAM_MPI_LIBBIN OUTPUT_VARIABLE OF16ext_FOAM_MPI_LIBBIN)

  macro (setup_exe_target_OF16ext targetname sources exename includes)
    #message(STATUS "target " ${targetname} ": includes=" ${includes})
#     link_directories(${OF16ext_LIB_DIR} 
#       ${OF16ext_FOAM_MPI_LIBBIN} 
#       ${OF16ext_METIS_LIB_DIR} 
#       ${OF16ext_PARMETIS_LIB_DIR}
#       ${OF16ext_SCOTCH_LIB_DIR}
#       ${OF16ext_MESQUITE_LIB_DIR}
#       )
    #SET(LIB_SEARCHFLAGS "-L${OF16ext_LIB_DIR} -L${OF16ext_FOAM_MPI_LIBBIN} -L${OF16ext_METIS_LIB_DIR} -L${OF16ext_PARMETIS_LIB_DIR} -L${OF16ext_SCOTCH_LIB_DIR} -L${OF16ext_MESQUITE_LIB_DIR}")
    add_executable(${targetname} ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF16ext_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF16ext_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/OpenFOAM-1.6-ext)
    target_link_libraries(${targetname} 
      ${OF16ext_LIB_DIR}/libOpenFOAM.so 
      ${OF16ext_FOAM_MPI_LIBBIN}/libPstream.so 
      #${OF16ext_METIS_LIB_DIR}/libmetis.a
      ${OF16ext_PARMETIS_LIB_DIR}/libparmetis.so
      ${OF16ext_SCOTCH_LIB_DIR}/libscotch.so
      ${OF16ext_MESQUITE_LIB_DIR}/libmesquite.so
      ${ARGN}) 
  endmacro()
  
  macro (setup_lib_target_OF16ext targetname sources exename includes)
    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    get_directory_property(temp LINK_DIRECTORIES)
    
    SET(LIB_SEARCHFLAGS "-L${OF16ext_LIB_DIR} -L${OF16ext_FOAM_MPI_LIBBIN} -L${OF16ext_METIS_LIB_DIR} -L${OF16ext_PARMETIS_LIB_DIR} -L${OF16ext_SCOTCH_LIB_DIR} -L${OF16ext_MESQUITE_LIB_DIR}")
#     link_directories(${OF16ext_LIB_DIR} 
#       ${OF16ext_FOAM_MPI_LIBBIN} 
#       ${OF16ext_METIS_LIB_DIR} 
#       ${OF16ext_PARMETIS_LIB_DIR}
#       ${OF16ext_SCOTCH_LIB_DIR}
#       ${OF16ext_MESQUITE_LIB_DIR}
#       )
    add_library(${targetname} SHARED ${sources})
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${includes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF16ext_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF16ext_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/OpenFOAM-1.6-ext)
    target_link_libraries(${targetname} ${ARGN}) 
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(OF16ext_FOUND TRUE)
ENDIF(OF16ext_DIR)