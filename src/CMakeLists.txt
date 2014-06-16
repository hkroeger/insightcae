project(insight)

OPTION(INSIGHT_OLD_BOOST_COMPAT "Omit extensions which are incompatible with older boost versions" OFF)
OPTION(INSIGHT_BUILD_WORKBENCH "Build the workbench GUI" ON)
OPTION(INSIGHT_BUILD_FREECAD "Build the FreeCAD extensions" ON)
OPTION(INSIGHT_BUILD_CAD "Build the ISCAD modeling language parser" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")

find_package(OpenFOAM-16ext REQUIRED)
find_package(OpenFOAM-21x REQUIRED)
find_package(OpenFOAM-22x REQUIRED)
find_package(OpenFOAM-22eng REQUIRED)
find_package(OpenFOAM-23x REQUIRED)
find_package(Armadillo REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
FIND_PACKAGE(SWIG REQUIRED)

# ADD_DEFINITIONS(
#     -std=c++11 # Or -std=c++0x
#     #-std=c++0x
#     # Other flags
# )

SET( _boost_TEST_VERSIONS ${Boost_ADDITIONAL_VERSIONS} "1.55.0" "1.41.0" "1.39.0"
	"1.38.0" "1.37.0"
        "1.36.1" "1.36.0" "1.35.1" "1.35.0" "1.35" "1.34.1" "1.34.0" "1.34"
        "1.33.1" "1.33.0" "1.33" )
find_package(Boost COMPONENTS system filesystem regex date_time thread iostreams python program_options REQUIRED)
message(STATUS "BOOST_INC=" ${Boost_INCLUDE_DIRS})


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(${ARMADILLO_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PYTHON_INCLUDE_DIRS})

SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Xlinker --add-needed -Wl,--no-as-needed") 

macro(setup_exe_target_OF PRJ_NAME sources of_include_dirs of_libs include_dirs libs)
  if (NOT OF_VERSIONS)
    if (OF16ext_FOUND)
      list (APPEND OF_VERSIONS OF16ext)
    endif()
    if (OF21x_FOUND)
      list (APPEND OF_VERSIONS OF21x)
    endif()
    if (OF22x_FOUND)
      list (APPEND OF_VERSIONS OF22x)
    endif()
    if (OF22eng_FOUND)
      list (APPEND OF_VERSIONS OF22eng)
    endif()
    if (OF23x_FOUND)
      list (APPEND OF_VERSIONS OF23x)
    endif()
  endif()
  message(STATUS ${OF_VERSIONS})
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    message(STATUS "Adding project ${PRJ_NAME}${_ofvers}")
    project(${PRJ_NAME}${_ofvers})
    
    #assemble list of full include paths
    set(${PRJ_NAME}_INCLUDE_DIRS ${include_dirs})
    foreach (_inc_dir ${of_include_dirs})
      list (APPEND ${PRJ_NAME}_INCLUDE_DIRS
      ${${_ofvers}_LIBSRC_DIR}/${_inc_dir}/lnInclude
      ${${_ofvers}_LIBSRC_DIR}/${_inc_dir}
      )
    endforeach()
    
    # dito for libs
    set(${PRJ_NAME}_LIBRARIES ${libs})
    foreach (_lib ${of_libs})
      list (APPEND ${PRJ_NAME}_LIBRARIES
      ${${_ofvers}_LIB_DIR}/lib${_lib}.so
      )
    endforeach()
    
    LIST(APPEND ${PRJ_NAME}_LIBRARIES ${${_ofvers}_LIBS})

    if (${_ofvers} STREQUAL "OF16ext")
      setup_exe_target_OF16ext(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF21x")
      setup_exe_target_OF21x(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF22x")
      setup_exe_target_OF22x(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF22eng")
      setup_exe_target_OF22eng(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF23x")
      setup_exe_target_OF23x(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
   endif()
  endforeach()
endmacro(setup_exe_target_OF)

macro(setup_lib_target_OF PRJ_NAME sources of_include_dirs of_libs include_dirs libs)
  if (NOT OF_VERSIONS)
    if (OF16ext_FOUND)
      list (APPEND OF_VERSIONS OF16ext)
    endif()
    if (OF21x_FOUND)
      list (APPEND OF_VERSIONS OF21x)
    endif()
    if (OF22x_FOUND)
      list (APPEND OF_VERSIONS OF22x)
    endif()
    if (OF22eng_FOUND)
      list (APPEND OF_VERSIONS OF22eng)
    endif()
    if (OF23x_FOUND)
      list (APPEND OF_VERSIONS OF23x)
    endif()
  endif()
  message(STATUS ${OF_VERSIONS})
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    message(STATUS "Adding project ${PRJ_NAME}${_ofvers}")
    project(${PRJ_NAME}${_ofvers})
    
    #assemble list of full include paths
    set(${PRJ_NAME}_INCLUDE_DIRS ${include_dirs})
    foreach (_inc_dir ${of_include_dirs} ${${_ofvers}_INC_DIRS})
      list (APPEND ${PRJ_NAME}_INCLUDE_DIRS
      ${${_ofvers}_LIBSRC_DIR}/${_inc_dir}/lnInclude
      ${${_ofvers}_LIBSRC_DIR}/${_inc_dir}
      )
    endforeach()
    
    # dito for libs
    set(${PRJ_NAME}_LIBRARIES ${libs})
    foreach (_lib ${of_libs})
      list (APPEND ${PRJ_NAME}_LIBRARIES
      ${${_ofvers}_LIB_DIR}/lib${_lib}.so
      )
    endforeach()
    
    LIST(APPEND ${PRJ_NAME}_LIBRARIES ${${_ofvers}_LIBS})

    if (${_ofvers} STREQUAL "OF16ext")
      setup_lib_target_OF16ext(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF21x")
      setup_lib_target_OF21x(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF22x")
      setup_lib_target_OF22x(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF22eng")
      setup_lib_target_OF22eng(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF23x")
      setup_lib_target_OF23x(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
   endif()
  endforeach()
endmacro(setup_lib_target_OF)


macro(install_script NAME SCRIPTFILE)
  file(GLOB COPY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${SCRIPTFILE})
    
  add_custom_target(${NAME} ALL
    COMMENT "Copying file: ${SCRIPTFILE}")

  set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPTFILE}")
  set(DST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SCRIPTFILE}")

  add_custom_command(
    TARGET ${NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
    )
endmacro(install_script)

macro(install_shared_file NAME SHAREDFILE DSTLOCATION)
  file(GLOB COPY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHAREDFILE})
    
  add_custom_target(${NAME} ALL
    COMMENT "Copying file: ${SHAREDFILE}")

  set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SHAREDFILE}")
  set(DST "${CMAKE_BINARY_DIR}/share/insight/${DSTLOCATION}/${SHAREDFILE}")

  add_custom_command(
    TARGET ${NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
    )
endmacro(install_shared_file)

macro(install_package_script PACKAGENAME)
  foreach (_src ${ARGN})
    file(GLOB COPY_FILES
      RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
      ${_src})
      
    add_custom_target(${PACKAGENAME}_${_src} ALL
      COMMENT "Copying file: ${_src}")

    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
    set(DST "${CMAKE_BINARY_DIR}/share/insight/python/Insight/${PACKAGENAME}/${_src}")

    add_custom_command(
      TARGET ${PACKAGENAME}_${_src}
      COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
      )
  endforeach()
endmacro(install_package_script)

macro (add_sources SRCS)
    #message("ARGC='${ARGC}' ARGV='${ARGV}' ARGN='${ARGN}'")
    file (RELATIVE_PATH _relPath "${PROJECT_SOURCE_DIR}" "${CMAKE_CURRENT_LIST_DIR}")
    #message("relPath='${_relPath}'")
    foreach (_src ${ARGN})
        if (_relPath)
            list (APPEND ${SRCS} "${_relPath}/${_src}")
        else()
            list (APPEND ${SRCS} "${_src}")
        endif()
    endforeach()
    if (_relPath)
        # propagate SRCS to parent directory
        set (${SRCS} ${${SRCS}} PARENT_SCOPE)
    endif()
endmacro()

macro (add_include_dir VARNAME)
list (APPEND ${VARNAME}
 ${CMAKE_CURRENT_LIST_DIR}
)
set (${VARNAME} ${${VARNAME}} PARENT_SCOPE)
endmacro()

add_subdirectory(refdata)
add_subdirectory(toolkit)
if (INSIGHT_BUILD_CAD)
  add_subdirectory(cad)
endif (INSIGHT_BUILD_CAD)
add_subdirectory(analyze)
if (INSIGHT_BUILD_WORKBENCH)
  add_subdirectory(workbench)
endif (INSIGHT_BUILD_WORKBENCH)
add_subdirectory(addons)
add_subdirectory(modules)
add_subdirectory(etc)
if (INSIGHT_BUILD_FREECAD)
  add_subdirectory(extensions/freecad)
endif (INSIGHT_BUILD_FREECAD)
add_subdirectory(extensions/openfoam)
add_subdirectory(extensions/python)
add_subdirectory(extensions/paraview)
add_subdirectory(extensions/code_aster)
