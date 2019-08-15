
set(PRJ scalarTransport2FunctionObject)

set(SRC 
 scalarTransport2.C
)

set(OF_INCLUDE_DIRS
    turbulenceModels
    turbulenceModels/incompressible/RAS
    transportModels
)

set(OF_LIBS 
)

set(INCLUDE_DIRS 
  ${CMAKE_CURRENT_LIST_DIR}
)

set(LIBS 
    uniof
)

set(OF_VERSIONS OFplus OFesi1806)

setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
