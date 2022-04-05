
set(PRJ extendedRigidBodyMotion)

set(SRC 
 extendedRigidBodyMotion.C
)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS 
)

set(INCLUDE_DIRS 
)

set(LIBS
)

set(OF_VERSIONS OFplus OFesi1806 OFesi1906 OFesi2112)
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
linkToolkit_OF_ShLib( ${PRJ} )
