#
# Generic detection of OpenFOAM / foam-extend installations
#
# Each supported OpenFOAM version is described by a small file in
# CMake/openfoam/versions/, which calls insight_define_openfoam(<ID> ...).
# insight_detect_openfoam_versions() reads all these definitions, looks for
# the installations and sets up the following variables for every detected
# version <ID> (e.g. OFesi2112):
#
#   <ID>_FOUND            TRUE, if the installation was found
#   <ID>_BASHRC           path of the OpenFOAM etc/bashrc (cache variable,
#                         can be set manually to point to an installation)
#   <ID>_DIR              installation root (WM_PROJECT_DIR)
#   <ID>_<envvar>         values of the OpenFOAM environment/wmake variables
#                         (WM_OPTIONS, FOAM_LIBBIN, FOAM_MPI, c++FLAGS, ...)
#   <ID>_CXX_FLAGS        compile flags for code compiled against this version
#   <ID>_LIBSRC_DIR       OpenFOAM src directory
#   <ID>_LIB_DIR          OpenFOAM library directory
#   <ID>_INCLUDE_PATHS    all lnInclude directories
#   <ID>_LIBRARIES        OpenFOAM libraries to link against
#   <ID>_EXTRA_LINK_LIBS  additional third party libraries to link against
#   <ID>_LIB_SEARCHFLAGS  linker search path flags
#   <ID>_INSIGHT_BIN/LIB  build output directories of InsightCAE targets
#   <ID>_INSIGHT_INSTALL_BIN/LIB  their install destinations
#   <ID>_ISCFG_BASHRC     generated InsightCAE environment script
#
# INSIGHT_OF_DETECTED_VERSIONS contains the IDs of all detected versions,
# ordered by OpenFOAM version number.
#
# Targets are created with setup_exe_target_for_OF(<ID> ...) and
# setup_lib_target_for_OF(<ID> ...).
#
# To support a new OpenFOAM version, add a file to CMake/openfoam/versions/.
# See the documentation of insight_define_openfoam() below.
#

include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/../OpenFOAMfuncs.cmake)

set(INSIGHT_OPENFOAM_SEARCH_ROOTS "$ENV{HOME};/opt" CACHE STRING
  "Base directories, below which the OpenFOAM installations are searched (the relative DIRS of the version definitions are appended)")

set(INSIGHT_OPENFOAM_VERSIONS_DIR "${CMAKE_CURRENT_LIST_DIR}/versions")


#
# Properties of the installation families
#
# Library dir and linker search dirs are templates: @VAR@ is replaced by the
# value of the detected variable <ID>_VAR. A search dir is skipped, if any of
# its referenced variables is empty or if the directory does not exist.
#
set(_INSIGHT_OF_COMMON_ENV_VARS
  WM_PROJECT WM_PROJECT_VERSION WM_OPTIONS
  FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN
  LINKLIBSO LINKEXE FOAM_MPI c++FLAGS
)

# OpenFOAM (Foundation, ESI, Engys)
set(_INSIGHT_OF_FAMILY_openfoam_ENV_VARS ${_INSIGHT_OF_COMMON_ENV_VARS})
set(_INSIGHT_OF_FAMILY_openfoam_LIB_DIR "@DIR@/platforms/@WM_OPTIONS@/lib")
set(_INSIGHT_OF_FAMILY_openfoam_SEARCH_DIRS
  "@LIB_DIR@" "@LIB_DIR@/@FOAM_MPI@" "@FOAM_EXT_LIBBIN@" "@SCOTCH_ROOT@/lib")
set(_INSIGHT_OF_FAMILY_openfoam_DEPLIBS
  "libfiniteVolume.so:Pstream" "libscotchDecomp.so:scotch")

# foam-extend (incl. OpenFOAM-1.6-ext)
set(_INSIGHT_OF_FAMILY_extend_ENV_VARS ${_INSIGHT_OF_COMMON_ENV_VARS}
  FOAM_MPI_LIBBIN
  METIS_LIB_DIR PARMETIS_LIB_DIR SCOTCH_LIB_DIR
  MESQUITE_LIB_DIR MESQUITE_INCLUDE_DIR
  PARMGRIDGEN_LIB_DIR PARMGRIDGEN_INCLUDE_DIR
)
set(_INSIGHT_OF_FAMILY_extend_LIB_DIR "@DIR@/lib/@WM_OPTIONS@")
set(_INSIGHT_OF_FAMILY_extend_SEARCH_DIRS
  "@LIB_DIR@" "@FOAM_MPI_LIBBIN@" "@METIS_LIB_DIR@" "@PARMETIS_LIB_DIR@"
  "@SCOTCH_LIB_DIR@" "@MESQUITE_LIB_DIR@")
set(_INSIGHT_OF_FAMILY_extend_DEPLIBS
  "libfiniteVolume.so:Pstream" "libscotchDecomp.so:scotch")




#
# insight_define_openfoam(<ID>
#   FAMILY <openfoam|extend>       installation layout, see above
#   FORK <name>                    adds -DOF_FORK_<name> (vanilla, esi, engys, extend)
#   OF_VERSION <nnnnnn>            adds -DOF_VERSION=<nnnnnn>, also determines the order
#   DIRS <dir>...                  installation directories to search. Relative paths
#                                  are searched below each INSIGHT_OPENFOAM_SEARCH_ROOTS entry
#   SHORTCUT <name>                name of the environment alias and insight.bashrc.<name>
#   ENV_ID <n>                     version number written into the .ofe file
#   [DEFINES <sym>...]             compile definitions (-D<sym>), default: <ID>
#   [LIBRARIES <lib>... | AUTO]    OpenFOAM libraries to link (without lib prefix and .so);
#                                  AUTO: all libraries built from the OpenFOAM src tree
#   [LIBRARIES_LIKE <otherID>]     take the library list of another version definition
#   [ADD_LIBRARIES <lib>...]       append libraries to the inherited list
#   [DEPLIBS <lib>:<pattern>...]   libraries to pick from the ldd output of FOAM_LIBBIN/<lib>,
#                                  default: family setting
#   [NO_DEPLIBS]                   do not look for dependency libraries
#   [EXTRA_LINK_LIBS <path>...]    additional libraries to link, may use @VAR@ references
#   [EXTRA_ENV_VARS <var>...]      additional variables to read from the environment
#   [INCPATH_SKIP <pattern>...]    skip source directories matching these patterns
#                                  when collecting lnInclude directories
#   [BASHRC_NAME <path>]           path of the bashrc relative to DIRS, default: etc/bashrc
#   [LEGACY_ENV_SCRIPT]            generate an environment script which sources the
#                                  OpenFOAM bashrc directly (old versions, no .ofe file)
# )
#
# Only records the definition, detection happens in insight_detect_openfoam_versions().
#
function(insight_define_openfoam ID)
  cmake_parse_arguments(D
    "NO_DEPLIBS;LEGACY_ENV_SCRIPT"
    "FAMILY;FORK;OF_VERSION;SHORTCUT;ENV_ID;LIBRARIES_LIKE;BASHRC_NAME"
    "DIRS;DEFINES;LIBRARIES;ADD_LIBRARIES;DEPLIBS;EXTRA_LINK_LIBS;EXTRA_ENV_VARS;INCPATH_SKIP"
    ${ARGN})

  if (D_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "insight_define_openfoam(${ID}): unknown arguments ${D_UNPARSED_ARGUMENTS}")
  endif()
  foreach(_req FAMILY FORK OF_VERSION DIRS SHORTCUT ENV_ID)
    if (NOT DEFINED D_${_req})
      message(FATAL_ERROR "insight_define_openfoam(${ID}): ${_req} is required")
    endif()
  endforeach()
  if (NOT DEFINED _INSIGHT_OF_FAMILY_${D_FAMILY}_LIB_DIR)
    message(FATAL_ERROR "insight_define_openfoam(${ID}): unknown FAMILY ${D_FAMILY}")
  endif()
  get_property(_defined GLOBAL PROPERTY INSIGHT_OF_DEFINED_VERSIONS)
  if (ID IN_LIST _defined)
    message(FATAL_ERROR "insight_define_openfoam(${ID}): defined twice")
  endif()

  if (NOT D_LIBRARIES AND NOT D_LIBRARIES_LIKE)
    # (also catches misspelled keywords, which cmake_parse_arguments
    #  silently appends to the preceding multi-value argument)
    message(FATAL_ERROR "insight_define_openfoam(${ID}): no LIBRARIES or LIBRARIES_LIKE given")
  endif()
  if (NOT D_DEFINES)
    set(D_DEFINES ${ID})
  endif()
  if (NOT D_BASHRC_NAME)
    set(D_BASHRC_NAME etc/bashrc)
  endif()
  if (NOT D_NO_DEPLIBS AND NOT D_DEPLIBS)
    set(D_DEPLIBS ${_INSIGHT_OF_FAMILY_${D_FAMILY}_DEPLIBS})
  endif()

  foreach(_key FAMILY FORK OF_VERSION DIRS SHORTCUT ENV_ID DEFINES
               LIBRARIES LIBRARIES_LIKE ADD_LIBRARIES DEPLIBS EXTRA_LINK_LIBS
               EXTRA_ENV_VARS INCPATH_SKIP BASHRC_NAME LEGACY_ENV_SCRIPT)
    set_property(GLOBAL PROPERTY INSIGHT_OF_${ID}_${_key} "${D_${_key}}")
  endforeach()
  set_property(GLOBAL APPEND PROPERTY INSIGHT_OF_DEFINED_VERSIONS ${ID})
endfunction()




# resolve the library list of a version definition (follows LIBRARIES_LIKE)
function(_insight_of_get_libraries ID OUTVAR)
  get_property(_libs GLOBAL PROPERTY INSIGHT_OF_${ID}_LIBRARIES)
  get_property(_like GLOBAL PROPERTY INSIGHT_OF_${ID}_LIBRARIES_LIKE)
  get_property(_add GLOBAL PROPERTY INSIGHT_OF_${ID}_ADD_LIBRARIES)
  if (_like)
    if (_libs)
      message(FATAL_ERROR "OpenFOAM version ${ID}: LIBRARIES and LIBRARIES_LIKE are mutually exclusive")
    endif()
    get_property(_defined GLOBAL PROPERTY INSIGHT_OF_DEFINED_VERSIONS)
    if (NOT _like IN_LIST _defined)
      message(FATAL_ERROR "OpenFOAM version ${ID}: LIBRARIES_LIKE refers to undefined version ${_like}")
    endif()
    _insight_of_get_libraries(${_like} _libs)
  endif()
  set(${OUTVAR} ${_libs} ${_add} PARENT_SCOPE)
endfunction()




# replace @VAR@ by the value of <ID>_VAR. Result is empty, if any referenced variable is empty.
function(_insight_of_expand ID TEMPLATE OUTVAR)
  set(_res "${TEMPLATE}")
  string(REGEX MATCHALL "@[A-Za-z0-9_+]+@" _refs "${TEMPLATE}")
  foreach(_ref ${_refs})
    string(REGEX REPLACE "^@(.*)@$" "\\1" _name "${_ref}")
    if ("${${ID}_${_name}}" STREQUAL "")
      set(${OUTVAR} "" PARENT_SCOPE)
      return()
    endif()
    string(REPLACE "${_ref}" "${${ID}_${_name}}" _res "${_res}")
  endforeach()
  set(${OUTVAR} "${_res}" PARENT_SCOPE)
endfunction()




# writes an environment script which sources the OpenFOAM bashrc directly (old versions)
macro(_insight_of_legacy_env_script ID SHORTCUT)
  set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
alias ${SHORTCUT}=\"source insight.bashrc.${SHORTCUT}\"
")
  set(${ID}_ISCFG_BASHRC ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/insight.bashrc.${SHORTCUT})
  create_script("insight.bashrc.${SHORTCUT}"
"source ${${ID}_BASHRC}

foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${${ID}_INSIGHT_LIB}
#- Clean LD_LIBRARY_PATH
cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
export PATH=$PATH:${${ID}_INSIGHT_BIN}
#- Clean PATH
cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
")
endmacro()




# Look for the installation of a defined version and set up all <ID>_* variables.
# (macro, because the variables have to be visible in the calling directory scope)
macro(_insight_of_detect ID)
  foreach(_key FAMILY FORK OF_VERSION DIRS SHORTCUT ENV_ID DEFINES DEPLIBS
               EXTRA_LINK_LIBS EXTRA_ENV_VARS INCPATH_SKIP BASHRC_NAME LEGACY_ENV_SCRIPT)
    get_property(_of_${_key} GLOBAL PROPERTY INSIGHT_OF_${ID}_${_key})
  endforeach()

  set(_of_hints)
  foreach(_d ${_of_DIRS})
    if (IS_ABSOLUTE "${_d}")
      list(APPEND _of_hints "${_d}")
    else()
      foreach(_root ${INSIGHT_OPENFOAM_SEARCH_ROOTS})
        list(APPEND _of_hints "${_root}/${_d}")
      endforeach()
    endif()
  endforeach()

  find_file(${ID}_BASHRC NAMES ${_of_BASHRC_NAME} HINTS ${_of_hints} NO_DEFAULT_PATH)
  mark_as_advanced(${ID}_BASHRC)

  set(${ID}_FOUND FALSE)

  if (${ID}_BASHRC)
    message(STATUS "OpenFOAM ${ID}: found ${${ID}_BASHRC}")

    get_filename_component(${ID}_ETC_DIR ${${ID}_BASHRC} PATH)
    get_filename_component(${ID}_DIR ${${ID}_ETC_DIR} PATH)
    set(${ID}_LIBSRC_DIR "${${ID}_DIR}/src")

    # environment and wmake variables
    detectEnvVars(${ID} ${_INSIGHT_OF_FAMILY_${_of_FAMILY}_ENV_VARS} ${_of_EXTRA_ENV_VARS})
    filterWarningFlags(${ID}_c++FLAGS)
    # remove the compiler command
    string(REGEX REPLACE "^[^ ]+" "" ${ID}_LINKLIBSO "${${ID}_LINKLIBSO}")
    string(REGEX REPLACE "^[^ ]+" "" ${ID}_LINKEXE "${${ID}_LINKEXE}")

    set(${ID}_CXX_FLAGS "${${ID}_c++FLAGS}")
    foreach(_def ${_of_DEFINES})
      string(APPEND ${ID}_CXX_FLAGS " -D${_def}")
    endforeach()
    string(APPEND ${ID}_CXX_FLAGS " -DOF_VERSION=${_of_OF_VERSION} -DOF_FORK_${_of_FORK}")

    _insight_of_expand(${ID} "${_INSIGHT_OF_FAMILY_${_of_FAMILY}_LIB_DIR}" ${ID}_LIB_DIR)

    # include paths
    detectIncPaths(${ID} ${_of_INCPATH_SKIP})

    # libraries
    _insight_of_get_libraries(${ID} _of_libs)
    if ("${_of_libs}" STREQUAL "AUTO")
      _ofCachedExec(${ID}_LIBRARIES ${ID}_libs
        COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFLibs ${${ID}_BASHRC}
        DEPENDS ${${ID}_FOAM_LIBBIN}
      )
    else()
      setOFlibvar(${ID} ${_of_libs})
    endif()
    foreach(_dl ${_of_DEPLIBS})
      string(REPLACE ":" ";" _dl "${_dl}")
      list(GET _dl 0 _dl_lib)
      list(GET _dl 1 _dl_pat)
      if (EXISTS "${${ID}_FOAM_LIBBIN}/${_dl_lib}")
        detectDepLib(${ID} "${${ID}_FOAM_LIBBIN}/${_dl_lib}" "${_dl_pat}")
      endif()
    endforeach()

    set(${ID}_EXTRA_LINK_LIBS)
    foreach(_l ${_of_EXTRA_LINK_LIBS})
      _insight_of_expand(${ID} "${_l}" _l)
      if (_l)
        list(APPEND ${ID}_EXTRA_LINK_LIBS "${_l}")
      endif()
    endforeach()

    set(${ID}_LIB_SEARCHFLAGS "")
    foreach(_sd ${_INSIGHT_OF_FAMILY_${_of_FAMILY}_SEARCH_DIRS})
      _insight_of_expand(${ID} "${_sd}" _sd)
      if (_sd AND IS_DIRECTORY "${_sd}")
        string(APPEND ${ID}_LIB_SEARCHFLAGS " -L${_sd}")
      endif()
    endforeach()
    string(STRIP "${${ID}_LIB_SEARCHFLAGS}" ${ID}_LIB_SEARCHFLAGS)

    # output locations of InsightCAE targets
    set(${ID}_INSIGHT_INSTALL_BIN "bin/${${ID}_WM_PROJECT}-${${ID}_WM_PROJECT_VERSION}")
    set(${ID}_INSIGHT_INSTALL_LIB "lib/${${ID}_WM_PROJECT}-${${ID}_WM_PROJECT_VERSION}")
    set(${ID}_INSIGHT_BIN "${CMAKE_BINARY_DIR}/${${ID}_INSIGHT_INSTALL_BIN}")
    set(${ID}_INSIGHT_LIB "${CMAKE_BINARY_DIR}/${${ID}_INSIGHT_INSTALL_LIB}")

    # environment scripts
    if (_of_LEGACY_ENV_SCRIPT)
      _insight_of_legacy_env_script(${ID} ${_of_SHORTCUT})
    else()
      addOFConfig(${ID} ${_of_SHORTCUT} ${_of_ENV_ID})
    endif()

    set(${ID}_FOUND TRUE)
    list(APPEND INSIGHT_OF_DETECTED_VERSIONS ${ID})
  else()
    message(STATUS "OpenFOAM ${ID}: not found")
  endif()
endmacro()




#
# Reads all version definitions and detects the installations.
# Sets INSIGHT_OF_DETECTED_VERSIONS (ordered by OpenFOAM version number).
#
macro(insight_detect_openfoam_versions)
  file(GLOB _of_version_files "${INSIGHT_OPENFOAM_VERSIONS_DIR}/*.cmake")
  list(SORT _of_version_files)
  foreach(_f ${_of_version_files})
    include(${_f})
  endforeach()

  # order by OF_VERSION, then by ID
  get_property(_of_defined GLOBAL PROPERTY INSIGHT_OF_DEFINED_VERSIONS)
  set(_of_sortkeys)
  foreach(_id ${_of_defined})
    get_property(_v GLOBAL PROPERTY INSIGHT_OF_${_id}_OF_VERSION)
    list(APPEND _of_sortkeys "${_v}:${_id}")
  endforeach()
  list(SORT _of_sortkeys)

  set(INSIGHT_OF_DETECTED_VERSIONS)
  foreach(_k ${_of_sortkeys})
    string(REGEX REPLACE "^[^:]*:" "" _id "${_k}")
    _insight_of_detect(${_id})
  endforeach()
endmacro()




#
# Create an executable or shared library target, which is compiled and linked
# against OpenFOAM version ID.
#
# setup_exe_target_for_OF(<ID> <targetname> <sources> <outputname> <includes> [<libs>...])
# setup_lib_target_for_OF(<ID> <targetname> <sources> <outputname> <includes> [<libs>...])
#
function(_insight_of_setup_target KIND ID targetname sources exename includes)
  if (NOT ${ID}_FOUND)
    message(FATAL_ERROR "OpenFOAM version ${ID} was not detected, cannot create target ${targetname}")
  endif()

  if (KIND STREQUAL "exe")
    add_executable(${targetname} ${sources})
    set(_linkflags "${${ID}_LINKEXE}")
  else()
    add_library(${targetname} SHARED ${sources})
    set(_linkflags "${${ID}_LINKLIBSO}")
  endif()

  set(_allincludes ${includes})
  list(APPEND _allincludes "${${ID}_INCLUDE_PATHS}")

  set_target_properties(${targetname} PROPERTIES
    INCLUDE_DIRECTORIES "${_allincludes}"
    COMPILE_FLAGS "${${ID}_CXX_FLAGS}"
    LINK_FLAGS "${_linkflags} ${${ID}_LIB_SEARCHFLAGS}"
    OUTPUT_NAME ${exename}
    BUILD_WITH_INSTALL_RPATH TRUE
  )
  target_link_libraries(${targetname} ${${ID}_LIBRARIES} ${ARGN} ${${ID}_EXTRA_LINK_LIBS})

  if (KIND STREQUAL "exe")
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${${ID}_INSIGHT_BIN})
    install(TARGETS ${targetname} RUNTIME DESTINATION ${${ID}_INSIGHT_INSTALL_BIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
  else()
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${${ID}_INSIGHT_LIB})
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${${ID}_INSIGHT_INSTALL_LIB} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
  endif()
endfunction()

function(setup_exe_target_for_OF ID targetname sources exename includes)
  _insight_of_setup_target(exe ${ID} ${targetname} "${sources}" "${exename}" "${includes}" ${ARGN})
endfunction()

function(setup_lib_target_for_OF ID targetname sources exename includes)
  _insight_of_setup_target(lib ${ID} ${targetname} "${sources}" "${exename}" "${includes}" ${ARGN})
endfunction()
