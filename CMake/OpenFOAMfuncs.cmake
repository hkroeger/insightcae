
option(INSIGHT_OF_REDETECT "Force re-detection of the OpenFOAM installations configuration (otherwise cached results are reused as long as the installations are unchanged)" OFF)

# Run an OpenFOAM detection helper script and cache its output.
# Sourcing the OpenFOAM bashrc and scanning the source tree is slow, so the result
# is stored in the cache and reused as long as the signature (command line plus
# modification times of the given files/directories) does not change.
#
# Usage: _ofCachedExec(<outvar> <cachekey> COMMAND <cmd...> DEPENDS <files...>)
function(_ofCachedExec OUTVAR CACHEKEY)
  cmake_parse_arguments(ARG "" "" "COMMAND;DEPENDS" ${ARGN})
  set(sig "${ARG_COMMAND}")
  foreach(_f ${ARG_COMMAND} ${ARG_DEPENDS})
    if (EXISTS "${_f}")
      file(TIMESTAMP "${_f}" _ts "%s" UTC)
      string(APPEND sig "|${_f}@${_ts}")
    endif()
  endforeach()
  string(MD5 sig "${sig}")

  if ( (NOT INSIGHT_OF_REDETECT) AND ("${_OFDETECT_${CACHEKEY}_SIG}" STREQUAL "${sig}") )
    # cache entries cannot hold newlines, so they are stored encoded
    string(REPLACE "@@NL@@" "\n" _out "${_OFDETECT_${CACHEKEY}_VALUE}")
    set(${OUTVAR} "${_out}" PARENT_SCOPE)
  else()
    execute_process(COMMAND ${ARG_COMMAND} OUTPUT_VARIABLE _out)
    string(REPLACE "\n" "@@NL@@" _enc "${_out}")
    set(_OFDETECT_${CACHEKEY}_VALUE "${_enc}" CACHE INTERNAL "")
    set(_OFDETECT_${CACHEKEY}_SIG "${sig}" CACHE INTERNAL "")
    set(${OUTVAR} "${_out}" PARENT_SCOPE)
  endif()
endfunction()


macro(setOFlibvar prefix)
  SET(${prefix}_LIBRARIES "")
   FOREACH(f ${ARGN})
    IF (EXISTS "${${prefix}_FOAM_LIBBIN}/lib${f}.so")
      LIST(APPEND ${prefix}_LIBRARIES "${${prefix}_FOAM_LIBBIN}/lib${f}.so")
    endif()
   ENDFOREACH(f)
#   set (${prefix}_LIBRARIES ${${prefix}_LIBRARIES} PARENT_SCOPE)
endmacro()


macro(detectEnvVars prefix)
  set(TARGETS "")
  foreach(_var ${ARGN})
    set(TARGETS "${TARGETS} print-${_var}")
  endforeach()

  _ofCachedExec(GETOFCFGVAR_OUTPUT ${prefix}_envvars
   COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVars
   ${${prefix}_BASHRC} ${TARGETS}
   DEPENDS ${${prefix}_ETC_DIR}/prefs.sh ${${prefix}_ETC_DIR}/config.sh ${${prefix}_ETC_DIR}/config
  )
  string(REPLACE "\n" ";" VALUES ${GETOFCFGVAR_OUTPUT})

  set(i 0)
  foreach(_var ${ARGN})
    list(GET VALUES ${i} ${prefix}_${_var})
    MATH(EXPR i "${i}+1")
  endforeach()
endmacro()


macro(filterWarningFlags VARNAME)
  string(REPLACE " " ";" ARGLIST ${${VARNAME}})
  #message(STATUS "before: ${ARGLIST}")
  list(FILTER ARGLIST EXCLUDE REGEX "^(-W[^n][^o][^-].*|$)")
  list(FILTER ARGLIST EXCLUDE REGEX "^(-ftemplate-depth.*|$)")
  #message(STATUS "after: ${ARGLIST}")
  list(JOIN ARGLIST " " ${VARNAME})
  #message(STATUS "after2: ${${VARNAME}}")
endmacro()

macro(detectDepLib prefix fromlib pattern)
 _ofCachedExec(addlibs "${prefix}_deplib_${fromlib}_${pattern}"
   COMMAND ${CMAKE_SOURCE_DIR}/CMake/findInDepLibs ${${prefix}_BASHRC} ${fromlib} "${pattern}"
 )
 #message(STATUS "detected for ${pattern} in dependencies of ${fromlib}: " ${addlibs})
 LIST(APPEND ${prefix}_LIBRARIES "${addlibs}")
endmacro()

macro(detectIncPaths prefix)
 # the dir timestamps change when sub-directories (e.g. modules) are added or removed
 _ofCachedExec(${prefix}_INCLUDE_PATHS ${prefix}_incpaths
   COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFincPath ${${prefix}_BASHRC} ${ARGN}
   DEPENDS ${${prefix}_DIR} ${${prefix}_DIR}/src ${${prefix}_DIR}/applications ${${prefix}_DIR}/modules ${${prefix}_FOAM_LIBBIN}
 )
 list(APPEND ${prefix}_INCLUDE_PATHS ${${prefix}_LIBSRC_DIR})
endmacro()



macro(addOFConfig prefix shortcut versionnumber)

    set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
alias ${shortcut}=\"source insight.bashrc.${shortcut}\"
")

    set(${prefix}_ISCFG_BASHRC ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/insight.bashrc.${shortcut})
  
    create_script("insight.bashrc.${shortcut}"
#"source ${${prefix}_BASHRC}
"source openfoam.bashrc.${shortcut}

export CURRENT_OFE=${prefix}
export CURRENT_OFE_FILE=$(basename $CURRENT_OFE)
foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$INSIGHT_INSTDIR/${${prefix}_INSIGHT_INSTALL_LIB}
#- Clean LD_LIBRARY_PATH
cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
export PATH=$PATH:$INSIGHT_INSTDIR/${${prefix}_INSIGHT_INSTALL_BIN}
#- Clean PATH
cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
")

    # do not install the following, just keep in bin dir. Installed variant is generated in superbuild
    file(WRITE "${CMAKE_BINARY_DIR}/share/insight/ofes.d/${shortcut}.ofe"
"<?xml version=\"1.0\" encoding=\"utf-8\"?>
<root>
<ofe label=\"${prefix}\" bashrc=\"insight.bashrc.${shortcut}\" version=\"${versionnumber}\"/>
</root>
")

endmacro()
