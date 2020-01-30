cmake_minimum_required(VERSION 3.5)

enable_language (C)
enable_language (CXX)
enable_language (Fortran OPTIONAL)

enable_testing()

if (DEFINED ENV{WM_PROJECT_VERSION})
 message(FATAL_ERROR "It seems that an OpenFOAM environment is loaded. Please build InsightCAE without any OpenFOAM environment variables set!")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")
list(APPEND CMAKE_LIBRARY_PATH $ENV{INSIGHT_THIRDPARTYDIR}/lib)
list(APPEND CMAKE_INCLUDE_PATH $ENV{INSIGHT_THIRDPARTYDIR}/include)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) # Relink needs to be avoided since it does not work for OF targets (multiple targets on a single file)
#set(SKIP_BUILD_RPATH 1)

OPTION(INSIGHT_BUILD_WORKBENCH "Build the workbench GUI (Qt is required)" ON)
OPTION(INSIGHT_BUILD_ADDONS "Build all present add-ons" ON)
OPTION(INSIGHT_BUILD_FREECAD "Install FreeCAD workbench" OFF)
OPTION(INSIGHT_BUILD_OPENFOAM "Build the OpenFOAM extensions (OpenFOAM is required)" ON)
OPTION(INSIGHT_BUILD_CODEASTER "Build the Code_Aster extensions (Code_Aster is required)" ON)
OPTION(INSIGHT_BUILD_CAD "Build the ISCAD modeling language parser (OCE is required)" ON)
OPTION(INSIGHT_BUILD_ISCAD "Build the ISCAD editor (OCE and Qt is required)" ON)
OPTION(INSIGHT_BUILD_TOOLKIT "Build the Insight toolkit and related tools" ON)
OPTION(INSIGHT_BUILD_PYTHONBINDINGS "Build python bindings for Insight toolkit and related tools (SWIG is required)" ON)
OPTION(INSIGHT_BUILD_WEBWORKBENCH "Build the Web-Workbench (Wt is required)" OFF)
OPTION(INSIGHT_BUILD_INSTALLDESKTOP "Install MIME types and Desktop integration (already during build)" OFF)
OPTION(INSIGHT_INSTALL_DESKTOP "Install MIME types and Desktop integration" ON)
OPTION(INSIGHT_FORCE_OCC "Use OCC, even if OCE is available" OFF)
option(BUILD_DOCUMENTATION "Use Doxygen to create the documentation (doxygen is required)" OFF)

if (INSIGHT_BUILD_ISCAD AND NOT INSIGHT_BUILD_CAD)
 message(FATAL_ERROR "It is not possible to build ISCAD without the CAD parser library. Please reconsider your option selection.")
endif()
if (INSIGHT_BUILD_WORKBENCH AND NOT INSIGHT_BUILD_TOOLKIT)
 message(FATAL_ERROR "It is not possible to build insight workbench without the toolkit library. Please reconsider your option selection.")
endif()

INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "InsightCAE")

### change package name according to build options
SET(CPACK_PACKAGE_NAME "insightcae")
if (INSIGHT_BUILD_CAD AND NOT INSIGHT_BUILD_OPENFOAM AND NOT INSIGHT_BUILD_CODEASTER)
 SET(CPACK_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-cad")
elseif (INSIGHT_BUILD_OPENFOAM AND NOT INSIGHT_BUILD_CODEASTER)
 SET(CPACK_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-cfd")
elseif (NOT INSIGHT_BUILD_OPENFOAM AND INSIGHT_BUILD_CODEASTER)
 SET(CPACK_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-fem")
elseif (INSIGHT_BUILD_WORKBENCH AND NOT INSIGHT_BUILD_OPENFOAM AND NOT INSIGHT_BUILD_CODEASTER)
 SET(CPACK_PACKAGE_NAME "${CPACK_PACKAGE_NAME}-eng")
endif()

set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME base)
set(INSIGHT_INSTALL_COMPONENT base)

set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
SET(CPACK_PACKAGE_VENDOR "Hannes Kroeger")
SET(CPACK_PACKAGE_CONTACT "hannes.kroeger@silentdynamics.de")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "a workbench for Computer-Aided Engineering")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "1")

execute_process(
  COMMAND git rev-list --count HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE git-rev
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "git-rev base: ${git-rev}")

string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME_LOWERCASE)
find_program(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")

SET(TARGETS_TO_INSTALL "")

add_subdirectory(src/desktop)

SET(CPACK_SET_DESTDIR ON)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


macro(create_script SCRIPTFILE SCR_CONTENT)
#   message(STATUS  "CTS: ${SCR_CONTENT}")
  set(SRC "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SCRIPTFILE}")
  file(WRITE "${SRC}" "${SCR_CONTENT}")
  install(FILES "${SRC}" DESTINATION bin COMPONENT ${INSIGHT_INSTALL_COMPONENT})
endmacro(create_script)

if (INSIGHT_BUILD_TOOLKIT)

    find_package(Armadillo REQUIRED)

    find_package(GSL REQUIRED)

    find_package(PythonLibs 2.7 REQUIRED)

    # modifies Boost_LIBRARIES for some reason, put it first!
    find_package(wt
        HINTS /opt/witty
    )

    find_package(Boost COMPONENTS
        regex
        system
        filesystem
        date_time
        timer
        thread
        iostreams
        python
        program_options
        REQUIRED
        )

    find_package(VTK
        HINTS /opt/paraview-offscreen/lib/cmake/paraview-5.6
        COMPONENTS
        vtkFiltersCore
        vtkFiltersGeneral
        vtkFiltersSources
        vtkCommonCore
        vtkIOLegacy
        vtkIOGeometry
        vtkIOExport
        vtkIOImage
        vtkIOXML
        vtkImagingCore
        vtkRenderingOpenGL2
        vtkRenderingAnnotation
        vtkFiltersExtraction
        REQUIRED
    )
    include(${VTK_USE_FILE})
endif()

include_directories(${ARMADILLO_INCLUDE_DIRS})
include_directories(${GSL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PYTHON_INCLUDE_DIRS})




function(JOIN VALUES GLUE OUTPUT)
  string (REPLACE ";" "${GLUE}" _TMP_STR "${VALUES}")
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()




if (INSIGHT_BUILD_OPENFOAM)
    SET(INSIGHT_OFES_VARCONTENT "")
    SET(INSIGHT_OF_ALIASES "")
    
    find_package(OpenFOAM-16ext)
    find_package(FOAM-extend-31)
    find_package(FOAM-extend-32)
    find_package(FOAM-extend-40)
    find_package(FOAM-extend-41)
    find_package(OpenFOAM-21x)
    find_package(OpenFOAM-22x)
    find_package(OpenFOAM-22eng)
    find_package(OpenFOAM-23x)
    find_package(OpenFOAM-24x)
    find_package(OpenFOAM-301)
    find_package(OpenFOAM-plus)
    find_package(OpenFOAM-dev)
    find_package(OpenFOAM-esi1806)
    find_package(OpenFOAM-esi1906)

    join("${INSIGHT_OFES_VARCONTENT}" "\n" INSIGHT_OFES)
    #message(STATUS "\"${INSIGHT_OFES}\"")
    
    create_script("insight.profile.OpenFOAM"
"INSIGHT_OFES=\"\"
${INSIGHT_OFES}
export INSIGHT_OFES
"
)
    create_script("insight.aliases.OpenFOAM"
"${INSIGHT_OF_ALIASES}
"
)
    create_script("insight.bashrc.OpenFOAM"
"source insight.profile.OpenFOAM
source insight.aliases.OpenFOAM
"
)
endif()




IF(INSIGHT_BUILD_PYTHONBINDINGS)
  FIND_PACKAGE(SWIG REQUIRED)
ENDIF()

set (CMAKE_CXX_STANDARD 11)
ADD_DEFINITIONS(
    -Wno-old-style-cast
    -ftemplate-depth=900
    -fpermissive 
    -Wno-deprecated 
    # Other flags
)




macro(version_list_OF)
  if (NOT OF_VERSIONS)
    if (OF16ext_FOUND)
      list (APPEND OF_VERSIONS OF16ext)
    endif()
    if (Fx31_FOUND)
      list (APPEND OF_VERSIONS Fx31)
    endif()
    if (Fx32_FOUND)
      list (APPEND OF_VERSIONS Fx32)
    endif()
    if (Fx40_FOUND)
      list (APPEND OF_VERSIONS Fx40)
    endif()
    if (Fx41_FOUND)
      list (APPEND OF_VERSIONS Fx41)
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
    if (OF24x_FOUND)
      list (APPEND OF_VERSIONS OF24x)
    endif()
    if (OF301_FOUND)
      list (APPEND OF_VERSIONS OF301)
    endif()
    if (OFplus_FOUND)
      list (APPEND OF_VERSIONS OFplus)
    endif()
    if (OFdev_FOUND)
      list (APPEND OF_VERSIONS OFdev)
    endif()
    if (OFesi1806_FOUND)
      list (APPEND OF_VERSIONS OFesi1806)
    endif()
    if (OFesi1906_FOUND)
      list (APPEND OF_VERSIONS OFesi1906)
    endif()
  endif()
endmacro(version_list_OF)




macro(setup_exe_target_OF PRJ_NAME sources of_include_dirs of_libs include_dirs libs is_of_targets)
  version_list_OF()
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
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
    
    foreach (_targ ${is_of_targets})
      list (APPEND ${PRJ_NAME}_LIBRARIES
      ${_targ}${_ofvers}
      )
      list (APPEND ${PRJ_NAME}_INCLUDE_DIRS
      ${${_targ}${_ofvers}_SOURCE_DIR}
      )
    endforeach()

    if (${_ofvers} STREQUAL "OF16ext")
      setup_exe_target_OF16ext(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "Fx31")
      setup_exe_target_Fx31(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "Fx32")
      setup_exe_target_Fx32(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "Fx40")
      setup_exe_target_Fx40(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "Fx41")
      setup_exe_target_Fx41(
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
    if (${_ofvers} STREQUAL "OF24x")
      setup_exe_target_OF24x(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF301")
      setup_exe_target_OF301(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OFplus")
      setup_exe_target_OFplus(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OFdev")
      setup_exe_target_OFdev(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OFesi1806")
      setup_exe_target_OFesi1806(
        ${PRJ_NAME}${_ofvers}
        "${sources}"
        "${PRJ_NAME}"
        "${${PRJ_NAME}_INCLUDE_DIRS}"
        "${${PRJ_NAME}_LIBRARIES}"
      )
    endif()
    if (${_ofvers} STREQUAL "OFesi1906")
      setup_exe_target_OFesi1906(
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




macro(setup_lib_target_OF PRJ_NAME sources of_include_dirs of_libs include_dirs libs is_of_targets)
  version_list_OF()
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
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
    
    #LIST(APPEND ${PRJ_NAME}_LIBRARIES ${${_ofvers}_LIBS})
    foreach (_targ ${is_of_targets})
      list (APPEND ${PRJ_NAME}_LIBRARIES
      ${_targ}${_ofvers}
      )
      list (APPEND ${PRJ_NAME}_INCLUDE_DIRS
      ${${_targ}${_ofvers}_SOURCE_DIR}
      )
    endforeach()

    if (${_ofvers} STREQUAL "OF16ext")
      setup_lib_target_OF16ext(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "Fx31")
      setup_lib_target_Fx31(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "Fx32")
      setup_lib_target_Fx32(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "Fx40")
      setup_lib_target_Fx40(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "Fx41")
      setup_lib_target_Fx41(
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
    if (${_ofvers} STREQUAL "OF24x")
      setup_lib_target_OF24x(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OF301")
      setup_lib_target_OF301(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OFplus")
      setup_lib_target_OFplus(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OFdev")
      setup_lib_target_OFdev(
	${PRJ_NAME}${_ofvers} 
	"${sources}" 
	"${PRJ_NAME}" 
	"${${PRJ_NAME}_INCLUDE_DIRS}"
	"${${PRJ_NAME}_LIBRARIES}"
      ) 
    endif()
    if (${_ofvers} STREQUAL "OFesi1806")
      setup_lib_target_OFesi1806(
        ${PRJ_NAME}${_ofvers}
        "${sources}"
        "${PRJ_NAME}"
        "${${PRJ_NAME}_INCLUDE_DIRS}"
        "${${PRJ_NAME}_LIBRARIES}"
      )
    endif()
    if (${_ofvers} STREQUAL "OFesi1906")
      setup_lib_target_OFesi1906(
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




macro(setup_test_OF PRJ_NAME cmd)
  version_list_OF()
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    add_test(NAME ${PRJ_NAME}${_ofvers} 
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${cmd} ${_ofvers} ${${_ofvers}_ISCFG_BASHRC} ${CMAKE_CURRENT_SOURCE_DIR}
    ) 
   endif()
  endforeach()
endmacro()


macro(casebuilder_test_OF TEST_NAME INPUT_FILE)
  version_list_OF()
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    set(TESTDIR ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}${_ofvers})
    add_test(NAME ${TEST_NAME}${_ofvers}
        COMMAND bash -c "mkdir -p ${TESTDIR} && cd ${TESTDIR} && rm -rf * && isofCaseBuilder -r ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE} --ofe ${_ofvers}"
    )
   endif()
  endforeach()
endmacro()


macro(insight_workflow_test TEST_NAME INPUT_FILE)
    set(TESTDIR ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME})
    add_test(NAME ${TEST_NAME}
        COMMAND bash -c "mkdir -p ${TESTDIR} && cd ${TESTDIR} && rm -rf * && cp ${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE} . && analyze ${INPUT_FILE}"
    )
endmacro()


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
  install(PROGRAMS ${SCRIPTFILE} ${SRC} DESTINATION bin COMPONENT ${INSIGHT_INSTALL_COMPONENT})
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
  install(FILES ${SHAREDFILE} DESTINATION share/insight/${DSTLOCATION} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
endmacro(install_shared_file)




macro(install_shared_directory NAME SHAREDDIR DSTLOCATION)
  file(GLOB COPY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${SHAREDDIR}
    ${SHAREDDIR}/*)

  add_custom_target(${NAME} ALL
    COMMENT "Copying directory: ${SHAREDDIR}")
  foreach(file ${COPY_FILES})
   set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SHAREDDIR}/${file}")
   set(DST "${CMAKE_BINARY_DIR}/share/insight/${DSTLOCATION}/${file}")

   add_custom_command(
     TARGET ${NAME}
     COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
     )
   install(FILES ${SHAREDDIR}/${file} DESTINATION share/insight/${DSTLOCATION} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
  endforeach()
endmacro(install_shared_directory)




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




########################################################################################
##
##   Python integration

macro (get_python_install_path PATHVARNAME INSTALL_PREFIX)
 execute_process(
    COMMAND python -c "from distutils import sysconfig; print( sysconfig.get_python_lib( plat_specific=True, prefix='${INSTALL_PREFIX}' ) )"
    OUTPUT_VARIABLE _ABS_PYTHON_MODULE_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
 )
 get_filename_component ( _ABS_PYTHON_MODULE_PATH ${_ABS_PYTHON_MODULE_PATH} ABSOLUTE )
 set ( ${PATHVARNAME} ${_ABS_PYTHON_MODULE_PATH} )
 file ( RELATIVE_PATH _REL_PYTHON_MODULE_PATH ${CMAKE_INSTALL_PREFIX} ${_ABS_PYTHON_MODULE_PATH} )
 set ( ${PATHVARNAME}_REL ${_REL_PYTHON_MODULE_PATH} )
endmacro()


get_python_install_path(INSIGHT_INSTALL_PYTHON_PATH ${CMAKE_INSTALL_PREFIX})
get_python_install_path(INSIGHT_PYTHON_PATH ${CMAKE_BINARY_DIR})


macro (add_pybindings TARGETNAME IFFILES LIBS)
  IF(INSIGHT_BUILD_PYTHONBINDINGS)
    INCLUDE(${SWIG_USE_FILE})
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    set(CMAKE_SWIG_OUTDIR ${INSIGHT_PYTHON_PATH}/Insight)
    SET(${TARGETNAME}_INTERFACES ${IFFILES})
    SET(CMAKE_SWIG_FLAGS "")
    SET_SOURCE_FILES_PROPERTIES(${${TARGETNAME}_INTERFACES} PROPERTIES CPLUSPLUS ON)
    SWIG_ADD_LIBRARY(${TARGETNAME} LANGUAGE python SOURCES ${${TARGETNAME}_INTERFACES})
    SWIG_LINK_LIBRARIES(${TARGETNAME} ${LIBS})
    set_target_properties(
        ${SWIG_MODULE_${TARGETNAME}_REAL_NAME}
        PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${INSIGHT_PYTHON_PATH}/Insight
        )
    INSTALL ( FILES ${INSIGHT_PYTHON_PATH}/Insight/_${TARGETNAME}.so DESTINATION ${INSIGHT_INSTALL_PYTHON_PATH_REL}/Insight COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    INSTALL ( FILES ${INSIGHT_PYTHON_PATH}/Insight/${TARGETNAME}.py DESTINATION ${INSIGHT_INSTALL_PYTHON_PATH_REL}/Insight COMPONENT ${INSIGHT_INSTALL_COMPONENT})
  ENDIF()
endmacro()


macro(install_package_script PACKAGENAME)
  foreach (_src ${ARGN})

    STRING(REPLACE "/" "_" TARGNAME ${PACKAGENAME}_${_src})
    STRING(REPLACE "." "_" TARGNAME ${TARGNAME})

    file(GLOB COPY_FILES
      RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
      ${_src})

    add_custom_target(${TARGNAME} ALL
      COMMENT "Copying file: ${_src}")

    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
    set(DST "${INSIGHT_PYTHON_PATH}/Insight/${PACKAGENAME}/${_src}")

#     message(STATUS "target name: ${TARGNAME}")
    add_custom_command(
      TARGET ${TARGNAME}
      COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
      )

    get_filename_component(DST_DIR ${INSIGHT_INSTALL_PYTHON_PATH_REL}/Insight/${PACKAGENAME}/${_src} DIRECTORY)
    install(
      FILES ${_src}
      DESTINATION ${DST_DIR} #share/insight/python/Insight/${PACKAGENAME}
      COMPONENT ${INSIGHT_INSTALL_COMPONENT}
      )
  endforeach()
endmacro(install_package_script)







set(IS_INSIGHTCAE_PROJECT 1) # activate depency of pdl-generated headers on pdl target
include(InsightCAE_CMakeFunctions)


include(CMakePackageConfigHelpers)

add_custom_target(cmake_dev_config ALL COMMENT "CMake development configuration")
set(INSIGHT_ConfigPackageLocation lib/cmake/insightcae)
SET(INSIGHT_CMAKE_CONFIG_DIR_BIN "${CMAKE_BINARY_DIR}/${INSIGHT_ConfigPackageLocation}")


add_subdirectory(src)
add_subdirectory(share)

SET(CPACK_PACKAGE_VERSION_PATCH ${git-rev})
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

set(PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
if(DPKG_PROGRAM)

  SET(CPACK_GENERATOR "DEB")
  set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  execute_process(
    COMMAND ${DPKG_PROGRAM} --print-architecture
    OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND lsb_release -rs
    OUTPUT_VARIABLE CPACK_DEBIAN_SYSTEM_DISTRIBUTION_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(
    COMMAND bash -c "lsb_release -as 2>/dev/null|sed -n '1p'|tr -d '\n'|tr '[:lower:][:upper:]' '[:lower:][:lower:]'"
    OUTPUT_VARIABLE CPACK_DEBIAN_SYSTEM_DISTRIBUTION_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(CPACK_PACKAGE_VERSION_PATCH ${CPACK_PACKAGE_VERSION_PATCH}~${CPACK_DEBIAN_SYSTEM_DISTRIBUTION_NAME}${CPACK_DEBIAN_SYSTEM_DISTRIBUTION_VERSION})
  set(PACKAGE_VERSION ${PACKAGE_VERSION}~${CPACK_DEBIAN_SYSTEM_DISTRIBUTION_NAME}${CPACK_DEBIAN_SYSTEM_DISTRIBUTION_VERSION})
  message(STATUS "Package version: ${PACKAGE_VERSION}")

  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "gnuplot,python-numpy,python-scipy,python-matplotlib,texlive-base,texlive-latex-extra,imagemagick,texlive-fonts-recommended,texlive-font-utils,xdg-utils,libssl1.0.0")
  if (INSIGHT_BUILD_OPENFOAM)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},openfoamesi1806-insightcae,paraview5-offscreen-insightcae,openmpi-bin")
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "paraview5-insightcae")
  endif()
#   if (INSIGHT_BUILD_CODEASTER)
#     set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},codeaster-insightcae")
#   endif()
  if (INSIGHT_BUILD_CAD OR INSIGHT_BUILD_FREECAD)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},liboce-insightcae,freecad-insightcae")
  endif()
  if (INSIGHT_BUILD_CAD)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},libdxflib (>=3.7.5)")
  endif()

else(DPKG_PROGRAM)

 if(EXISTS /etc/redhat-release)
   SET(CPACK_GENERATOR "RPM")
   execute_process(
     COMMAND uname -m
     OUTPUT_VARIABLE CPACK_RPM_PACKAGE_ARCHITECTURE
     OUTPUT_STRIP_TRAILING_WHITESPACE
   )
   set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}-${CPACK_PACKAGE_VERSION_PATCH}.${CPACK_RPM_PACKAGE_ARCHITECTURE}")
  set(CPACK_RPM_PACKAGE_REQUIRES "/usr/bin/gnuplot-wx,/usr/bin/pdflatex,/usr/bin/convert,/usr/lib64/libcrypto.so") #numpy,scipy,matplotlib
  if (INSIGHT_BUILD_OPENFOAM)
    set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},/usr/bin/pvbatch")
#   if (INSIGHT_BUILD_CODEASTER)
#     set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, codeaster-insightcae")
#   endif()
  endif()
  if (INSIGHT_BUILD_CAD OR INSIGHT_BUILD_FREECAD)
    set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES},/usr/bin/FreeCAD")
  endif()
#  if (INSIGHT_BUILD_CAD)
#    set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, libdxflib >=3.7.5")
#  endif()
 endif()

endif(DPKG_PROGRAM)


write_basic_package_version_file(
  "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEConfigVersion.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)
add_custom_command(
 TARGET cmake_dev_config
 COMMAND ${CMAKE_COMMAND} -E
     copy ${CMAKE_SOURCE_DIR}/CMake/InsightCAE_CMakeFunctions.cmake ${INSIGHT_CMAKE_CONFIG_DIR_BIN}
 COMMAND ${CMAKE_COMMAND} -E
     copy ${CMAKE_SOURCE_DIR}/gen-sets.py ${CMAKE_BINARY_DIR}/bin
)
install(
  FILES ${CMAKE_SOURCE_DIR}/CMake/InsightCAE_CMakeFunctions.cmake
  DESTINATION ${INSIGHT_ConfigPackageLocation}
  COMPONENT ${INSIGHT_INSTALL_COMPONENT}
)
install(
    FILES ${CMAKE_SOURCE_DIR}/gen-sets.py
    DESTINATION bin
    COMPONENT ${INSIGHT_INSTALL_COMPONENT}
)

set(_inccmd "")
foreach(_f ${TARGETS_TO_INSTALL})
    set(_inccmd "${_inccmd}\ninclude(\"\${CMAKE_CURRENT_LIST_DIR}/${_f}\")")
endforeach()
configure_file(CMake/InsightCAEConfig.cmake.in
  "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEConfig.cmake"
  @ONLY #COPYONLY
)


install(
  FILES
    "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEConfig.cmake"
    "${INSIGHT_CMAKE_CONFIG_DIR_BIN}/InsightCAEConfigVersion.cmake"
  DESTINATION
    ${INSIGHT_ConfigPackageLocation}
  COMPONENT
    ${INSIGHT_INSTALL_COMPONENT}
)

INCLUDE(CPack)
