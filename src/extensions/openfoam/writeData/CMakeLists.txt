set(PRJ writeData)

set(SRC 
 writeData.C 
 writeDataFunctionObject.C 
)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers
)

set(OF_LIBS 
#finiteVolume 
)

set(INCLUDE_DIRS 
)

set(LIBS 
)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
