
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

  execute_process(
   COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVars
   ${${prefix}_BASHRC} ${TARGETS}
   OUTPUT_VARIABLE GETOFCFGVAR_OUTPUT
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
  #message(STATUS "after: ${ARGLIST}")
  list(JOIN ARGLIST " " ${VARNAME})
  #message(STATUS "after2: ${${VARNAME}}")
endmacro()

macro(detectDepLib prefix fromlib pattern)
 execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/findInDepLibs ${${prefix}_BASHRC} ${fromlib} "${pattern}" OUTPUT_VARIABLE addlibs)
 #message(STATUS "detected for ${pattern} in dependencies of ${fromlib}: " ${addlibs})
 LIST(APPEND ${prefix}_LIBRARIES "${addlibs}")
endmacro()

macro(detectIncPaths prefix)
 execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFincPath ${${prefix}_BASHRC} ${ARGN} OUTPUT_VARIABLE ${prefix}_INCLUDE_PATHS)
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
