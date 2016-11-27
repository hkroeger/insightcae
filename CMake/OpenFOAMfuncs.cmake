
macro(setOFlibvar prefix)
  SET(${prefix}_LIBRARIES "")
   FOREACH(f ${ARGN})
    IF (EXISTS "${${prefix}_FOAM_LIBBIN}/lib${f}.so")
      LIST(APPEND ${prefix}_LIBRARIES "${${prefix}_FOAM_LIBBIN}/lib${f}.so")
    endif()
   ENDFOREACH(f)
   set (${prefix}_LIBRARIES ${${prefix}_LIBRARIES} PARENT_SCOPE)
endmacro()

macro(detectEnvVar prefix varname outvarname)
 execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/getOFCfgVar ${${prefix}_BASHRC} print-${varname} OUTPUT_VARIABLE ${prefix}_${outvarname})
 #message(STATUS "Detected value of env var " ${varname} "=" ${${prefix}_${outvarname}})
endmacro()

macro(detectEnvVars prefix)
 FOREACH(f ${ARGN})
  detectEnvVar(${prefix} ${f} ${f})
 ENDFOREACH(f)
endmacro()

macro(detectDepLib prefix fromlib pattern)
 execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/findInDepLibs ${${prefix}_BASHRC} ${fromlib} "${pattern}" OUTPUT_VARIABLE addlibs)
 message(STATUS "detected for ${pattern} in dependencies of ${fromlib}: " ${addlibs})
 LIST(APPEND ${prefix}_LIBRARIES "${addlibs}")
endmacro()

macro(detectIncPaths prefix)
 execute_process(COMMAND ${CMAKE_SOURCE_DIR}/CMake/printOFincPath ${${prefix}_BASHRC} OUTPUT_VARIABLE ${prefix}_INCLUDE_PATHS)
endmacro()



macro(addOFConfig prefix shortcut versionnumber)
  list(APPEND INSIGHT_OFES_VARCONTENT "${prefix}@`find \\\${PATH//:/ } -maxdepth 1 -name insight.bashrc.${shortcut} -print -quit`#${versionnumber}")
  set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
alias ${shortcut}=\"source insight.bashrc.${shortcut}\"
")

  set(${prefix}_ISCFG_BASHRC ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/insight.bashrc.${shortcut})
  
  create_script("insight.bashrc.${shortcut}"
"source ${${prefix}_BASHRC}

foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${${prefix}_INSIGHT_LIB}
#- Clean LD_LIBRARY_PATH
cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
export PATH=$PATH:${${prefix}_INSIGHT_BIN}
#- Clean PATH
cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
")
endmacro()
