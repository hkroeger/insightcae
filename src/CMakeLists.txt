project(insight)
cmake_minimum_required(VERSION 2.6)

OPTION(INSIGHT_BUILD_WORKBENCH "Build the workbench GUI" ON)
OPTION(INSIGHT_BUILD_FREECAD "Build the FreeCAD extensions" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")

find_package(OpenFOAM-16ext REQUIRED)
find_package(OpenFOAM-22x REQUIRED)
find_package(Armadillo REQUIRED)

SET( _boost_TEST_VERSIONS ${Boost_ADDITIONAL_VERSIONS} "1.55.0" "1.41.0" "1.39.0"
	"1.38.0" "1.37.0"
        "1.36.1" "1.36.0" "1.35.1" "1.35.0" "1.35" "1.34.1" "1.34.0" "1.34"
        "1.33.1" "1.33.0" "1.33" )
find_package(Boost COMPONENTS system filesystem regex date_time thread REQUIRED)
message(STATUS "BOOST_INC=" ${Boost_INCLUDE_DIRS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(${ARMADILLO_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Xlinker --add-needed -Wl,--no-as-needed") 

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

add_subdirectory(toolkit)
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