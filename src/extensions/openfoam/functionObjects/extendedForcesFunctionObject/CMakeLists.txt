set(PRJ extendedForcesFunctionObject)

set(SRC 
 ${PRJ}.C
)

set(OF_INCLUDE_DIRS
  transportModels
  TurbulenceModels/phaseCompressible
)

set(OF_LIBS 
)

set(INCLUDE_DIRS 
)

set(LIBS 
 uniof Boost::headers
)

set(OFesi2112_OF_LIBS
  compressibleTwoPhaseSystem
)

setup_lib_target_OF(${PRJ} "${SRC}"
    "${OF_INCLUDE_DIRS}" "${OF_LIBS}"
    "${INCLUDE_DIRS}" "${LIBS}" "forceSources")
 
