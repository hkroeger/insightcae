cmake_minimum_required(VERSION 3.5)

cmake_policy(SET CMP0078 NEW)
cmake_policy(SET CMP0086 NEW)

project(InsightCAE)

enable_language (C)
enable_language (CXX)
enable_language (Fortran OPTIONAL)


if(WIN32)
 add_link_options(LINKER:--export-all-symbols)
endif()

enable_testing()

if (DEFINED ENV{WM_PROJECT_VERSION})
 message(FATAL_ERROR "It seems that an OpenFOAM environment is loaded. Please build InsightCAE without any OpenFOAM environment variables set!")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")
list(APPEND CMAKE_LIBRARY_PATH $ENV{INSIGHT_THIRDPARTYDIR}/lib)
list(APPEND CMAKE_INCLUDE_PATH $ENV{INSIGHT_THIRDPARTYDIR}/include)


OPTION(INSIGHT_BUILD_ADDONS "Build all present add-ons" ON)
OPTION(INSIGHT_BUILD_FREECAD "Install FreeCAD workbench" OFF)
OPTION(INSIGHT_BUILD_OPENFOAM "Build the OpenFOAM extensions (OpenFOAM is required)" ON)
OPTION(INSIGHT_BUILD_CODEASTER "Build the Code_Aster extensions" ON)
OPTION(INSIGHT_BUILD_MEDREADER "Build the Code_Aster MED reader plugin for Paraview (libmed is required to be installed)" OFF)
OPTION(INSIGHT_BUILD_CAD "Build the ISCAD modeling language parser (OCE is required)" ON)
OPTION(INSIGHT_BUILD_TOOLKIT "Build the Insight toolkit and related tools" ON)
OPTION(INSIGHT_BUILD_PYTHONBINDINGS "Build python bindings for Insight toolkit and related tools (SWIG is required)" ON)
OPTION(INSIGHT_BUILD_WEBWORKBENCH "Build the Web-Workbench (Wt is required)" OFF)
OPTION(INSIGHT_BUILD_INSTALLDESKTOP "Install MIME types and Desktop integration (already during build)" OFF)
OPTION(INSIGHT_BUILD_TESTS "Build all executables for testing" OFF)
OPTION(INSIGHT_DEVELOPMENT_FILES "create CMake package files to build other projects with CMake and InsightCAE as a dependency" ON)
OPTION(INSIGHT_INSTALL_DESKTOP "Install MIME types and Desktop integration" ON)
OPTION(INSIGHT_FORCE_OCC "Use OCC, even if OCE is available" OFF)

OPTION(INSIGHT_GNUPLOT_CHARTRENDERER "Enable gnuplot for rendering charts" ON)
OPTION(INSIGHT_MATPLOTLIB_CHARTRENDERER "Enable matplotlib for rendering charts" OFF)

SET(INSIGHT_SUPERBUILD "" CACHE STRING "Path to superbuild (build directory). If set, use dependencies from that superbuild")

option(BUILD_DOCUMENTATION "Use Doxygen to create the documentation (doxygen is required)" OFF)

option(INSIGHT_BUILD_GUICOMPONENTS "Enable the GUI tools (workbench, CAD, case builder..., Qt is required)" ON)
if (INSIGHT_BUILD_GUICOMPONENTS)
 OPTION(INSIGHT_BUILD_WORKBENCH "Build the workbench GUI (Qt is required)" ON)
 OPTION(INSIGHT_BUILD_ISCAD "Build the ISCAD editor (OCE and Qt is required)" ON)
endif()

if (CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_WRAPPER "/opt/mxe/usr/bin/i686-w64-mingw32.shared-cmake" CACHE STRING "required in MXE cross build, cmake is called through wrapper script there")
  list(APPEND VTK_CMAKE_ADDARGS "-DVTKCompileTools_DIR=${VTKCompileTools_DIR}")
else()
  set(CMAKE_WRAPPER ${CMAKE_COMMAND})
endif()

if (INSIGHT_BUILD_ISCAD AND NOT INSIGHT_BUILD_CAD)
 message(FATAL_ERROR "It is not possible to build ISCAD without the CAD parser library. Please reconsider your option selection.")
endif()
if (INSIGHT_BUILD_WORKBENCH AND NOT INSIGHT_BUILD_TOOLKIT)
 message(FATAL_ERROR "It is not possible to build insight workbench without the toolkit library. Please reconsider your option selection.")
endif()

if (NOT ${INSIGHT_SUPERBUILD} STREQUAL "")
 include (${INSIGHT_SUPERBUILD}/InsightSuperbuildDependencies.cmake)
ENDIF()

if (NOT VTK_ONSCREEN_DIR)
  message(STATUS "VTK_ONSCREEN_DIR not defined. Set to VTK_DIR")
  set(VTK_ONSCREEN_DIR ${VTK_DIR})
endif()


### determine combined git revision
SET(INSIGHT_VERSION_MAJOR "0")
SET(INSIGHT_VERSION_MINOR "0")
SET(INSIGHT_VERSION_PATCH "0")
SET(INSIGHT_BRANCH "none")



if (EXISTS ${CMAKE_SOURCE_DIR}/.git)
    execute_process(
      COMMAND git describe
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE git-rev
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE ret
    )
    if (ret EQUAL "0")
     execute_process(
       COMMAND git rev-parse --abbrev-ref HEAD
       WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
       OUTPUT_VARIABLE INSIGHT_BRANCH
       OUTPUT_STRIP_TRAILING_WHITESPACE
     )
     if (INSIGHT_BRANCH STREQUAL "HEAD")
       execute_process(
         COMMAND bash -c "basename $(git log -1 --pretty=%D|sed -e 's/,/\\n/g'|grep \"origin/\"|tail -n 1)"
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
         OUTPUT_VARIABLE INSIGHT_BRANCH
         OUTPUT_STRIP_TRAILING_WHITESPACE
       )
     endif()
     message(STATUS "InsightCAE git branch: ${INSIGHT_BRANCH}")
     message(STATUS "InsightCAE git describe: ${git-rev}")
     STRING(REGEX REPLACE
         "^([0-9]+)\\.([0-9]+)[-]*([^ ]+|)\$"
         "\\1;\\2;\\3" VERSIONS ${git-rev})
 
     LIST(GET VERSIONS 0 INSIGHT_VERSION_MAJOR)
     LIST(GET VERSIONS 1 INSIGHT_VERSION_MINOR)
     LIST(GET VERSIONS 2 INSIGHT_VERSION_PATCH)
     message(STATUS "InsightCAE version major=${INSIGHT_VERSION_MAJOR} / minor=${INSIGHT_VERSION_MINOR} / patch=${INSIGHT_VERSION_PATCH}")
   else()
     message(STATUS "git describe failed")
   endif()
endif()

add_definitions(
    -DINSIGHT_VERSION_MAJOR=${INSIGHT_VERSION_MAJOR}
    -DINSIGHT_VERSION_MINOR=${INSIGHT_VERSION_MINOR}
    -DINSIGHT_VERSION_PATCH=${INSIGHT_VERSION_PATCH}
    -DINSIGHT_BRANCH=${INSIGHT_BRANCH}
    )



INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "InsightCAE")

### change package name according to build options
SET(CPACK_PACKAGE_NAME "insightcae")

set(CPACK_DEB_COMPONENT_INSTALL OFF)
set(CPACK_RPM_COMPONENT_INSTALL OFF)
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME base)
set(INSIGHT_INSTALL_COMPONENT base)

set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
SET(CPACK_PACKAGE_VENDOR "Hannes Kroeger")
SET(CPACK_PACKAGE_CONTACT "hannes.kroeger@silentdynamics.de")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A workbench for Computer-Aided Engineering")
set(CPACK_PACKAGE_VERSION_MAJOR ${INSIGHT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${INSIGHT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${INSIGHT_VERSION_PATCH})


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
  message(STATUS "Creating ${SRC}")
  file(WRITE "${SRC}" "${SCR_CONTENT}")
  install(FILES "${SRC}" DESTINATION bin COMPONENT ${INSIGHT_INSTALL_COMPONENT})
endmacro(create_script)


set(IS_INSIGHTCAE_PROJECT 1) # activate depency of pdl-generated headers on pdl target
include(InsightCAE_CMakeFunctions)


include(CMakePackageConfigHelpers)

add_custom_target(cmake_dev_config ALL COMMENT "CMake development configuration")
set(INSIGHT_ConfigPackageLocation lib/cmake/insightcae)
SET(INSIGHT_CMAKE_CONFIG_DIR_BIN "${CMAKE_BINARY_DIR}/${INSIGHT_ConfigPackageLocation}")


if (INSIGHT_BUILD_TOOLKIT)

    find_package(Armadillo REQUIRED)

    find_package(GSL REQUIRED)

    find_package(PythonLibs)
    if (NOT PYTHONLIBS_FOUND)
        message(STATUS "No python libraries found - python analyses will be unavailable")
    endif()

    # modifies Boost_LIBRARIES for some reason, put it first!
    find_package(wt)

    set(BOOST_COMPONENTS
        regex
        system
        filesystem
        date_time
        timer
        thread
        iostreams
        program_options
        )
    if (PYTHONLIBS_FOUND)
        list(APPEND BOOST_COMPONENTS python3)
    endif()

    find_package(Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED
        )

    add_subdirectory(CMake/vtkTargets/vtkOffscreen)
    add_subdirectory(CMake/vtkTargets/vtkOnscreen)

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
    find_package(OpenFOAM-esi2112)

    create_script("insight.aliases.OpenFOAM"
"${INSIGHT_OF_ALIASES}
"
)

endif()


find_package(Intl REQUIRED)

include("CMake/Gettext_helpers.cmake")

macro(setup_gettext TARGET_NAME GETTEXT_DOMAIN)
    set(GETTEXT_TARGET "${TARGET_NAME}-gettext")
    set(GETTEXT_OUTPUT_DIR "locale")
    set(GETTEXT_LANGUAGES "en" "de")

    target_compile_definitions(${TARGET_NAME} PRIVATE "GETTEXT_DOMAIN=\"${GETTEXT_DOMAIN}\"")
    target_compile_definitions(${TARGET_NAME} PRIVATE "GETTEXT_OUTPUT_DIR=\"${GETTEXT_OUTPUT_DIR}\"")

    CONFIGURE_GETTEXT(
     DOMAIN ${GETTEXT_DOMAIN}
     TARGET_NAME ${GETTEXT_TARGET}
     SOURCES ${ARGN}
     POTFILE_DESTINATION ${GETTEXT_OUTPUT_DIR}
     XGETTEXT_ARGS
      "--keyword=_"
      "--add-comments=TRANSLATORS:"
      "--package-name=${GETTEXT_DOMAIN}"
      "--package-version=${PROJECT_VERSION}"
      "--copyright-holder=hkroeger"
     LANGUAGES ${GETTEXT_LANGUAGES}
     BUILD_DESTINATION ${CMAKE_BINARY_DIR}/share/insight/${GETTEXT_OUTPUT_DIR}
     INSTALL_DESTINATION share/insight/${GETTEXT_OUTPUT_DIR}
     INSTALL_COMPONENT ${INSIGHT_INSTALL_COMPONENT}
     ALL
    )
endmacro()

IF(INSIGHT_BUILD_PYTHONBINDINGS)
  FIND_PACKAGE(SWIG REQUIRED)
  set(UseSWIG_TARGET_NAME_PREFERENCE STANDARD)
  #set(USE_TARGET_INCLUDE_DIRECTORIES TRUE)
  INCLUDE(${SWIG_USE_FILE})
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
    if (OFesi2112_FOUND)
      list (APPEND OF_VERSIONS OFesi2112)
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
    if (${_ofvers} STREQUAL "OFesi2112")
      setup_exe_target_OFesi2112(
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
    foreach (_lib ${${_ofvers}_OF_LIBS})
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
    if (${_ofvers} STREQUAL "OFesi2112")
      setup_lib_target_OFesi2112(
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

macro(add_test_OF TESTNAME CMD)
    version_list_OF()
    foreach (_ofvers ${OF_VERSIONS})
     if (${_ofvers}_FOUND)
      add_test(
          NAME ${TESTNAME}${_ofvers}
          COMMAND bash -lc "source ${${_ofvers}_ISCFG_BASHRC};${CMD} ${_ofvers} ${ARGN}"
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
  set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPTFILE}")
  set(DST "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SCRIPTFILE}")

  add_custom_command(
    MAIN_DEPENDENCY ${SRC}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
    OUTPUT ${DST}
    )
  install(PROGRAMS ${SCRIPTFILE} ${SRC} DESTINATION bin COMPONENT ${INSIGHT_INSTALL_COMPONENT})

  add_library(${NAME} INTERFACE ${DST})

endmacro(install_script)




macro(install_shared_file NAME SHAREDFILE DSTLOCATION)

  file(GLOB COPY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHAREDFILE})
  get_filename_component(FNAME ${SHAREDFILE} NAME)
  set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SHAREDFILE}")
  set(DST "${CMAKE_BINARY_DIR}/share/insight/${DSTLOCATION}/${FNAME}")

  add_custom_command(
    MAIN_DEPENDENCY ${SRC}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
    OUTPUT ${DST}
  )
  install(FILES ${SHAREDFILE} DESTINATION share/insight/${DSTLOCATION} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

  add_library(${NAME} INTERFACE ${DST})

endmacro(install_shared_file)


macro(install_shared_file2 SHAREDFILE DSTLOCATION)

  string(REGEX REPLACE "[,\.;/\\ ]" "_" NAME ${SHAREDFILE})
  file(GLOB COPY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${SHAREDFILE})
  get_filename_component(FNAME ${SHAREDFILE} NAME)
  set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SHAREDFILE}")
  set(DST "${CMAKE_BINARY_DIR}/share/insight/${DSTLOCATION}/${FNAME}")

  add_custom_command(
    MAIN_DEPENDENCY ${SRC}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
    OUTPUT ${DST}
  )
  install(FILES ${SHAREDFILE} DESTINATION share/insight/${DSTLOCATION} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

  add_library(${NAME} INTERFACE ${DST})

endmacro(install_shared_file2)



macro(install_shared_directory NAME SHAREDDIR DSTLOCATION)

  file(GLOB COPY_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${SHAREDDIR}
    ${SHAREDDIR}/*)

  SET(DSTS "")
  foreach(file ${COPY_FILES})
   set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SHAREDDIR}/${file}")
   set(DST "${CMAKE_BINARY_DIR}/share/insight/${DSTLOCATION}/${file}")

   add_custom_command(
     MAIN_DEPENDENCY ${SRC}
     COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
     OUTPUT ${DST}
     )
   LIST(APPEND DSTS ${DST})
   install(FILES ${SHAREDDIR}/${file} DESTINATION share/insight/${DSTLOCATION} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
  endforeach()

  add_library(${NAME} INTERFACE ${DSTS})

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
         COMMAND python3 -c "from distutils import sysconfig; print( sysconfig.get_python_lib( plat_specific=False, prefix='${INSTALL_PREFIX}' ) )"
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

set(CMAKE_SWIG_OUTDIR ${INSIGHT_PYTHON_PATH}/Insight)

macro (add_pybindings TARGETNAME IFFILE LIBS)
  IF(INSIGHT_BUILD_PYTHONBINDINGS)
    set(PYTARGET ${TARGETNAME}_python)
    set_property(SOURCE ${IFFILE} PROPERTY CPLUSPLUS ON)
    set_property(SOURCE ${IFFILE} PROPERTY INCLUDE_DIRECTORIES ${toolkit_SOURCE_DIR})
    set_property(SOURCE ${IFFILE} PROPERTY SWIG_MODULE_NAME ${TARGETNAME})
    swig_add_library(${PYTARGET}
        LANGUAGE python
        SOURCES ${IFFILE})
    target_link_libraries(${PYTARGET} ${LIBS})
    #linkToolkitVtk(${PYTARGET} Offscreen) # causes clash in workbench (links onscreen and offscreen at the same time)
    set_target_properties(${PYTARGET}
        PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SWIG_OUTDIR}
        )
    INSTALL ( FILES ${CMAKE_SWIG_OUTDIR}/_${PYTARGET}.so DESTINATION ${INSIGHT_INSTALL_PYTHON_PATH_REL}/Insight COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    INSTALL ( FILES ${CMAKE_SWIG_OUTDIR}/${TARGETNAME}.py DESTINATION ${INSIGHT_INSTALL_PYTHON_PATH_REL}/Insight COMPONENT ${INSIGHT_INSTALL_COMPONENT})
  ENDIF()
endmacro()


macro(install_package_script PACKAGENAME)

  set(DSTS "")
  foreach (_src ${ARGN})

    STRING(REPLACE "/" "_" TARGNAME ${PACKAGENAME}_${_src})
    STRING(REPLACE "." "_" TARGNAME ${TARGNAME})
    file(GLOB COPY_FILES
      RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
      ${_src})
    set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/${_src}")
    set(DST "${INSIGHT_PYTHON_PATH}/Insight/${PACKAGENAME}/${_src}")

    add_custom_command(
      MAIN_DEPENDENCY ${SRC}
      COMMAND ${CMAKE_COMMAND} -E copy ${SRC} ${DST}
      OUTPUT ${DST}
    )
    LIST(APPEND DSTS ${DST})

    get_filename_component(DST_DIR ${INSIGHT_INSTALL_PYTHON_PATH_REL}/Insight/${PACKAGENAME}/${_src} DIRECTORY)
    install(
      FILES ${_src}
      DESTINATION ${DST_DIR} #share/insight/python/Insight/${PACKAGENAME}
      COMPONENT ${INSIGHT_INSTALL_COMPONENT}
      )
  endforeach()

  add_library(${PACKAGENAME}_files INTERFACE ${DSTS})

endmacro(install_package_script)









add_subdirectory(src)
add_subdirectory(share)
add_subdirectory(test)



SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")

set(PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
if(DPKG_PROGRAM)

  SET(CPACK_GENERATOR "DEB")
  set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
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
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "insightcae-dependencies,libpoppler-cpp0v5")

else(DPKG_PROGRAM)

 if(EXISTS /etc/redhat-release)
   SET(CPACK_GENERATOR "RPM")
   execute_process(
     COMMAND uname -m
     OUTPUT_VARIABLE CPACK_RPM_PACKAGE_ARCHITECTURE
     OUTPUT_STRIP_TRAILING_WHITESPACE
   )
   set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}-${CPACK_PACKAGE_VERSION_PATCH}.${CPACK_RPM_PACKAGE_ARCHITECTURE}")
  set(CPACK_RPM_PACKAGE_REQUIRES "/opt/insightcae/bin/insight_setthirdpartyenv.sh")
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
