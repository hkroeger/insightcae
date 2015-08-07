project(insight)

list(APPEND CMAKE_LIBRARY_PATH $ENV{INSIGHT_THIRDPARTYDIR}/lib)
list(APPEND CMAKE_INCLUDE_PATH $ENV{INSIGHT_THIRDPARTYDIR}/include)

message (STATUS ${CMAKE_LIBRARY_PATH})

INCLUDE(InstallRequiredSystemLibraries)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "InsightCAE")
SET(CPACK_PACKAGE_NAME "insightcae")
SET(CPACK_PACKAGE_VENDOR "Hannes Kroeger")
SET(CPACK_PACKAGE_CONTACT "hannes@kroegeronline.net")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "a workbench for Computer-Aided Engineering")
#SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/ReadMe.txt")
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")
# IF(WIN32 AND NOT UNIX)
#   # There is a bug in NSI that does not handle full unix paths properly. Make
#   # sure there is at least one set of four (4) backlasshes.
#   SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
#   SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
#   SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} My Famous Project")
#   SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.my-project-home-page.org")
#   SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.my-personal-home-page.com")
#   SET(CPACK_NSIS_CONTACT "me@my-personal-home-page.com")
#   SET(CPACK_NSIS_MODIFY_PATH ON)
# ELSE(WIN32 AND NOT UNIX)
#   SET(CPACK_STRIP_FILES "bin/MyExecutable")
#   SET(CPACK_SOURCE_STRIP_FILES "")
# ENDIF(WIN32 AND NOT UNIX)
# SET(CPACK_PACKAGE_EXECUTABLES "MyExecutable" "My Executable")
SET(CPACK_GENERATOR "TGZ;DEB")
set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
INCLUDE(CPack)

OPTION(INSIGHT_OLD_BOOST_COMPAT "Omit extensions which are incompatible with older boost versions" OFF)
OPTION(INSIGHT_BUILD_WORKBENCH "Build the workbench GUI" ON)
OPTION(INSIGHT_BUILD_FREECAD "Build the FreeCAD extensions" ON)
OPTION(INSIGHT_BUILD_CAD "Build the ISCAD modeling language parser" ON)
OPTION(INSIGHT_BUILD_TOOLKIT "Build the Insight toolkit and related tools" ON)
OPTION(INSIGHT_BUILD_PYTHONBINDINGS "Build python bindings for Insight toolkit
and related tools (requires SWIG)" ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMake/")

find_package(OpenFOAM-16ext REQUIRED)
find_package(FOAM-extend-31 REQUIRED)
find_package(OpenFOAM-21x REQUIRED)
find_package(OpenFOAM-22x REQUIRED)
find_package(OpenFOAM-22eng REQUIRED)
find_package(OpenFOAM-23x REQUIRED)
find_package(Armadillo REQUIRED)
find_package(GSL REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

IF(INSIGHT_BUILD_PYTHONBINDINGS)
  FIND_PACKAGE(SWIG REQUIRED)
ENDIF()


ADD_DEFINITIONS(
    #-std=c++11 # Or -std=c++0x
    -std=c++0x
    # Other flags
)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -Wno-deprecated -Wno-old-style-cast")

SET( _boost_TEST_VERSIONS ${Boost_ADDITIONAL_VERSIONS} "1.55.0" "1.49.0" "1.41.0" "1.39.0"
	"1.38.0" "1.37.0"
        "1.36.1" "1.36.0" "1.35.1" "1.35.0" "1.35" "1.34.1" "1.34.0" "1.34"
        "1.33.1" "1.33.0" "1.33" )
find_package(Boost COMPONENTS system filesystem regex date_time thread iostreams python program_options REQUIRED)
message(STATUS "BOOST_INC=" ${Boost_INCLUDE_DIRS})


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(${ARMADILLO_INCLUDE_DIRS})
include_directories(${GSL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

#SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Xlinker --add-needed -Wl,--no-as-needed") 

macro(setup_exe_target_OF PRJ_NAME sources of_include_dirs of_libs include_dirs libs)
  if (NOT OF_VERSIONS)
    if (OF16ext_FOUND)
      list (APPEND OF_VERSIONS OF16ext)
    endif()
    if (Fx31_FOUND)
      list (APPEND OF_VERSIONS Fx31)
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
    if (${_ofvers} STREQUAL "Fx31")
      setup_exe_target_Fx31(
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
    if (Fx31_FOUND)
      list (APPEND OF_VERSIONS Fx31)
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
    if (${_ofvers} STREQUAL "Fx31")
      setup_lib_target_Fx31(
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
  install(PROGRAMS ${SCRIPTFILE} ${SRC} DESTINATION bin)
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
  install(FILES ${SHAREDFILE} DESTINATION share/insight/${DSTLOCATION} )
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

macro (add_pybindings TARGETNAME IFFILES LIBS)
  IF(INSIGHT_BUILD_PYTHONBINDINGS)
    INCLUDE(${SWIG_USE_FILE})
    INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
    set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/share/insight/python/Insight)
    SET(${TARGETNAME}_INTERFACES ${IFFILES})
    SET(CMAKE_SWIG_FLAGS "")
    SET_SOURCE_FILES_PROPERTIES(${${TARGETNAME}_INTERFACES} PROPERTIES CPLUSPLUS ON)
#     set(INCS "")
#     foreach (_src ${INCLUDEDIRS})
#      set(INCS "${INCS} -I${_src}")
#     endforeach()
#     message(STATUS ${INCS})
#     SET_SOURCE_FILES_PROPERTIES(${${TARGETNAME}_INTERFACES} PROPERTIES SWIG_FLAGS ${INCS})
    #SET_SOURCE_FILES_PROPERTIES(${toolkit_INTERFACES} PROPERTIES SWIG_FLAGS "-builtin")
    SWIG_ADD_MODULE(${TARGETNAME} python ${${TARGETNAME}_INTERFACES})
    SWIG_LINK_LIBRARIES(${TARGETNAME} ${LIBS})
    set_target_properties(${SWIG_MODULE_${TARGETNAME}_REAL_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/insight/python/Insight)
    
#     SET(SWIG_RUNTIME swigpyrun.h)
#     ADD_CUSTOM_COMMAND(
#     OUTPUT ${SWIG_RUNTIME}
#     COMMAND ${SWIG_EXECUTABLE} -python -external-runtime swigpyrun.h
#     )
#     ADD_CUSTOM_TARGET(swig-runtime-h ALL DEPENDS ${SWIG_RUNTIME})
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
add_subdirectory(refdata)
add_subdirectory(test)

IF(INSIGHT_BUILD_TOOLKIT)
  add_subdirectory(toolkit)
  add_subdirectory(analyze)
  if (INSIGHT_BUILD_WORKBENCH)
    add_subdirectory(workbench)
  endif (INSIGHT_BUILD_WORKBENCH)
endif()

if (INSIGHT_BUILD_CAD)
  add_subdirectory(cad)
endif (INSIGHT_BUILD_CAD)
add_subdirectory(addons)

IF(INSIGHT_BUILD_TOOLKIT)
  add_subdirectory(modules)
ENDIF()

add_subdirectory(etc)
if (INSIGHT_BUILD_FREECAD)
  add_subdirectory(extensions/freecad)
endif (INSIGHT_BUILD_FREECAD)
add_subdirectory(extensions/openfoam)
add_subdirectory(extensions/python)
add_subdirectory(extensions/paraview)
add_subdirectory(extensions/code_aster)
add_subdirectory(extensions/taskjuggler)
add_subdirectory(extensions/dakota)

#-- Add an Option to toggle the generation of the API documentation
option(BUILD_DOCUMENTATION "Use Doxygen to create the documentation" ON)
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
