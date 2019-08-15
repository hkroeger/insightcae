set(PRJ extendedForcesFunctionObject)

set(SRC 
 ${PRJ}.C
)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS 
)

set(INCLUDE_DIRS 
)

set(LIBS 
 uniof
)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
 
