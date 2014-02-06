project(insight)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

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
add_subdirectory(workbench)
add_subdirectory(addons)
add_subdirectory(modules)
add_subdirectory(etc)
add_subdirectory(extensions/freecad)
add_subdirectory(extensions/openfoam)