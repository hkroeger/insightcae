project(insight)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")
list(APPEND CMAKE_LIBRARY_PATH $ENV{INSIGHT_THIRDPARTYDIR}/lib)
list(APPEND CMAKE_INCLUDE_PATH $ENV{INSIGHT_THIRDPARTYDIR}/include)


OPTION(INSIGHT_BUILD_WORKBENCH "Build the workbench GUI (Qt4 is required)" ON)
OPTION(INSIGHT_BUILD_ADDONS "Build all present add-ons" ON)
OPTION(INSIGHT_BUILD_FREECAD "Install FreeCAD workbench" OFF)
OPTION(INSIGHT_BUILD_OPENFOAM "Build the OpenFOAM extensions (OpenFOAM is required)" ON)
OPTION(INSIGHT_BUILD_CODEASTER "Build the Code_Aster extensions (Code_Aster is required)" ON)
OPTION(INSIGHT_BUILD_CAD "Build the ISCAD modeling language parser (OCE is required)" ON)
OPTION(INSIGHT_BUILD_ISCAD "Build the ISCAD editor (OCE and Qt is required)" ON)
OPTION(INSIGHT_BUILD_TOOLKIT "Build the Insight toolkit and related tools" ON)
OPTION(INSIGHT_BUILD_PYTHONBINDINGS "Build python bindings for Insight toolkit and related tools (SWIG is required)" ON)
OPTION(INSIGHT_BUILD_WEBWORKBENCH "Build the Web-Workbench (Wt is required)" ON)
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

set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME base)
set(INSIGHT_INSTALL_COMPONENT base)

set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
SET(CPACK_PACKAGE_VENDOR "Hannes Kroeger")
SET(CPACK_PACKAGE_CONTACT "hannes.kroeger@silentdynamics.de")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "a workbench for Computer-Aided Engineering")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
execute_process(
  COMMAND git rev-list --count HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE CPACK_PACKAGE_VERSION_PATCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")


string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME_LOWERCASE)
find_program(DPKG_PROGRAM dpkg DOC "dpkg program of Debian-based systems")
set(PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
if(DPKG_PROGRAM)
  SET(CPACK_GENERATOR "DEB")
  set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  execute_process(
    COMMAND ${DPKG_PROGRAM} --print-architecture
    OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "gnuplot,python-numpy,python-scipy,python-matplotlib,texlive-base,texlive-latex-extra,imagemagick")
  if (INSIGHT_BUILD_OPENFOAM)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},openfoam23x-insightcae")
  endif()
#   if (INSIGHT_BUILD_CODEASTER)
#     set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},codeaster-insightcae")
#   endif()
  if (INSIGHT_BUILD_CAD OR INSIGHT_BUILD_FREECAD)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},oce-insightcae,freecad-insightcae")
  endif()
  if (INSIGHT_BUILD_CAD)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS},libdxflib (>=3.7.5),libsalomegeom (>=7.2.30)")
  endif()
else(DPKG_PROGRAM)
  SET(CPACK_GENERATOR "TGZ")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${PACKAGE_VERSION}_${CMAKE_SYSTEM_NAME}")
endif(DPKG_PROGRAM)

SET(CPACK_SET_DESTDIR ON)



set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


macro(create_script SCRIPTFILE SCR_CONTENT)
#   message(STATUS  "CTS: ${SCR_CONTENT}")
  set(SRC "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SCRIPTFILE}")
  file(WRITE "${SRC}" "${SCR_CONTENT}")
  install(FILES "${SRC}" DESTINATION bin COMPONENT ${INSIGHT_INSTALL_COMPONENT})
endmacro(create_script)

find_package(Armadillo REQUIRED)
find_package(GSL REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
SET( _boost_TEST_VERSIONS ${Boost_ADDITIONAL_VERSIONS} "1.55.0" "1.49.0" "1.41.0" "1.39.0"
	"1.38.0" "1.37.0"
        "1.36.1" "1.36.0" "1.35.1" "1.35.0" "1.35" "1.34.1" "1.34.0" "1.34"
        "1.33.1" "1.33.0" "1.33" )
find_package(Boost COMPONENTS system filesystem regex date_time thread iostreams python program_options REQUIRED)


include_directories(${ARMADILLO_INCLUDE_DIRS})
include_directories(${GSL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PYTHON_INCLUDE_DIRS})

if (INSIGHT_BUILD_WEBWORKBENCH)
    find_package(Wt)
endif()

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
    find_package(OpenFOAM-21x)
    find_package(OpenFOAM-22x)
    find_package(OpenFOAM-22eng)
    find_package(OpenFOAM-23x)
    find_package(OpenFOAM-plus)
    find_package(OpenFOAM-dev)
    
    join("${INSIGHT_OFES_VARCONTENT}" ":" INSIGHT_OFES)
    message(STATUS "\"${INSIGHT_OFES}\"")
    
    create_script("insight.bashrc.OpenFOAM"
"export INSIGHT_OFES=\"${INSIGHT_OFES}\"
${INSIGHT_OF_ALIASES}
"
)
endif()


IF(INSIGHT_BUILD_PYTHONBINDINGS)
  FIND_PACKAGE(SWIG REQUIRED)
ENDIF()


ADD_DEFINITIONS(
    #-std=c++11 # Or -std=c++0x
    -std=c++0x
    -ftemplate-depth=900
    -fpermissive 
    -Wno-deprecated 
    -Wno-old-style-cast
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
    if (OFplus_FOUND)
      list (APPEND OF_VERSIONS OFplus)
    endif()
    if (OFdev_FOUND)
      list (APPEND OF_VERSIONS OFdev)
    endif()
  endif()
endmacro(version_list_OF)


macro(setup_exe_target_OF PRJ_NAME sources of_include_dirs of_libs include_dirs libs is_of_targets)
  version_list_OF()
#   if (NOT OF_VERSIONS)
#     if (OF16ext_FOUND)
#       list (APPEND OF_VERSIONS OF16ext)
#     endif()
#     if (Fx31_FOUND)
#       list (APPEND OF_VERSIONS Fx31)
#     endif()
#     if (Fx32_FOUND)
#       list (APPEND OF_VERSIONS Fx32)
#     endif()
#     if (Fx40_FOUND)
#       list (APPEND OF_VERSIONS Fx40)
#     endif()
#     if (OF21x_FOUND)
#       list (APPEND OF_VERSIONS OF21x)
#     endif()
#     if (OF22x_FOUND)
#       list (APPEND OF_VERSIONS OF22x)
#     endif()
#     if (OF22eng_FOUND)
#       list (APPEND OF_VERSIONS OF22eng)
#     endif()
#     if (OF23x_FOUND)
#       list (APPEND OF_VERSIONS OF23x)
#     endif()
#     if (OFplus_FOUND)
#       list (APPEND OF_VERSIONS OFplus)
#     endif()
#     if (OFdev_FOUND)
#       list (APPEND OF_VERSIONS OFdev)
#     endif()
#   endif()
#   message(STATUS ${OF_VERSIONS})
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    message(STATUS "Adding program target ${PRJ_NAME}${_ofvers}")
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
   endif()
  endforeach()
endmacro(setup_exe_target_OF)




macro(setup_lib_target_OF PRJ_NAME sources of_include_dirs of_libs include_dirs libs is_of_targets)
  version_list_OF()
#   message(STATUS ${OF_VERSIONS})
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    message(STATUS "Adding lib target ${PRJ_NAME}${_ofvers}")
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
   endif()
  endforeach()
endmacro(setup_lib_target_OF)


macro(setup_test_OF PRJ_NAME cmd)
  version_list_OF()
#   message(STATUS ${OF_VERSIONS})
  foreach (_ofvers ${OF_VERSIONS})
   if (${_ofvers}_FOUND)
    message(STATUS "Adding OpenFOAM test ${PRJ_NAME}${_ofvers} ${_ofvers}_ISCFG_BASHRC ${${_ofvers}_ISCFG_BASHRC}")
#     project(${PRJ_NAME}${_ofvers})
        
    add_test(NAME ${PRJ_NAME}${_ofvers} 
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${cmd} ${_ofvers} ${${_ofvers}_ISCFG_BASHRC} ${CMAKE_CURRENT_SOURCE_DIR}
    ) 
   endif()
  endforeach()
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
    install(FILES ${_src} DESTINATION share/insight/python/Insight/${PACKAGENAME} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
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

macro (add_pybindings TARGETNAME IFFILES LIBS)
  IF(INSIGHT_BUILD_PYTHONBINDINGS)
    INCLUDE(${SWIG_USE_FILE})
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/share/insight/python/Insight)
    SET(${TARGETNAME}_INTERFACES ${IFFILES})
    SET(CMAKE_SWIG_FLAGS "")
    SET_SOURCE_FILES_PROPERTIES(${${TARGETNAME}_INTERFACES} PROPERTIES CPLUSPLUS ON)
    SWIG_ADD_MODULE(${TARGETNAME} python ${${TARGETNAME}_INTERFACES})
    SWIG_LINK_LIBRARIES(${TARGETNAME} ${LIBS})
    set_target_properties(${SWIG_MODULE_${TARGETNAME}_REAL_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/insight/python/Insight)
  ENDIF()
endmacro()

macro (add_PDL TARGETNAME HEADERS)
  foreach (_hdr ${HEADERS})
    get_filename_component(BN ${_hdr} NAME_WE)
    message(STATUS ${_hdr} ${BN})
    list (APPEND ${TARGETNAME}_TIMESTAMPS ${BN}_pdl.timestamp)
    ADD_CUSTOM_COMMAND( OUTPUT ${BN}_pdl.timestamp
			COMMAND ${CMAKE_SOURCE_DIR}/gen-sets.py "${_hdr}" "${CMAKE_BINARY_DIR}/bin/pdl"
			#COMMAND ${CMAKE_BINARY_DIR}/bin/pdl ${BN}__*.pdl
			COMMAND touch ${BN}_pdl.timestamp
			DEPENDS pdl ${CMAKE_SOURCE_DIR}/gen-sets.py ${_hdr}
			COMMENT "Generating source code from PDL in header ${_hdr}" )
			
  endforeach()
  ADD_CUSTOM_TARGET( ${TARGETNAME}_PDLGenerator DEPENDS ${${TARGETNAME}_TIMESTAMPS}
		    COMMENT "Checking if PDL re-generation is required" )
  ADD_DEPENDENCIES( ${TARGETNAME} ${TARGETNAME}_PDLGenerator )
endmacro(add_PDL)


add_subdirectory(pdl)
add_subdirectory(test)
add_subdirectory(refdata)
add_subdirectory(etc)
add_subdirectory(extensions/python)
add_subdirectory(extensions/paraview)
add_subdirectory(extensions/taskjuggler)
add_subdirectory(extensions/dakota)

IF (INSIGHT_BUILD_CODEASTER)
    add_subdirectory(extensions/code_aster)
ENDIF()

if (INSIGHT_BUILD_OPENFOAM)
 add_subdirectory(extensions/openfoam)
ENDIF()

IF(INSIGHT_BUILD_TOOLKIT)
  add_subdirectory(toolkit)
  add_subdirectory(analyze)
  add_subdirectory(modules)
  if (INSIGHT_BUILD_WORKBENCH)
    add_subdirectory(toolkit_gui)
    add_subdirectory(workbench)
  endif (INSIGHT_BUILD_WORKBENCH)
  IF(INSIGHT_BUILD_WEBWORKBENCH)
    add_subdirectory(webworkbench)
  ENDIF()
  if (INSIGHT_BUILD_CAD)
    add_subdirectory(cad)
  endif(INSIGHT_BUILD_CAD)
endif()


if (INSIGHT_BUILD_FREECAD)
  add_subdirectory(extensions/freecad)
endif (INSIGHT_BUILD_FREECAD)

IF (INSIGHT_BUILD_ADDONS)
  add_subdirectory(addons)
ENDIF()

#-- Add an Option to toggle the generation of the API documentation
if(BUILD_DOCUMENTATION)
  FIND_PACKAGE(Doxygen)
  if (NOT DOXYGEN_FOUND)
    message(FATAL_ERROR
      "Doxygen is needed to build the documentation. Please install it correctly")
  endif()
  #-- Configure the Template Doxyfile for our specific project
  configure_file(Doxyfile.in
                 ${PROJECT_BINARY_DIR}/Doxyfile  @ONLY IMMEDIATE)
  #-- Add a custom target to run Doxygen when ever the project is built
  add_custom_target (doc #ALL
                     COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                     SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
  # IF you do NOT want the documentation to be generated EVERY time you build the project
  # then leave out the 'ALL' keyword from the above command.
endif()

INCLUDE(CPack)