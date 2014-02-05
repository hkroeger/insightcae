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
  
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-LINKLIBSO OUTPUT_VARIABLE OF16ext_LINKLIBSO)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-LINKEXE OUTPUT_VARIABLE OF16ext_LINKEXE)
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${OF16ext_BASHRC} print-FOAM_MPI_LIBBIN OUTPUT_VARIABLE OF16ext_FOAM_MPI_LIBBIN)

  link_directories(${OF16ext_LIB_DIR} 
    ${OF16ext_FOAM_MPI_LIBBIN} 
    ${OF16ext_METIS_LIB_DIR} 
    ${OF16ext_PARMETIS_LIB_DIR}
    ${OF16ext_SCOTCH_LIB_DIR}
    ${OF16ext_MESQUITE_LIB_DIR}
    )

  macro (setup_exe_target_OF16ext targetname exename)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OF16ext_CXX_FLAGS}")
    set(LINK_FLAGS "${LINK_FLAGS} ${OF16ext_LINKEXE}")
    add_executable(${targetname} ${${targetname}_SOURCES})
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/OpenFOAM-1.6-ext)
    target_link_libraries(${targetname} ${ARGN}) 
  endmacro()
  
  macro (setup_lib_target_OF16ext targetname exename)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OF16ext_CXX_FLAGS}")
    set(LINK_FLAGS "${LINK_FLAGS} ${OF16ext_LINKEXE}")
    add_library(${targetname} SHARED ${${targetname}_SOURCES})
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/OpenFOAM-1.6-ext)
    target_link_libraries(${targetname} ${ARGN}) 
  endmacro()
  
  SET(OF16ext_FOUND TRUE)
ENDIF(OF16ext_DIR)