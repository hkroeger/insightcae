
set(PRJ extendedRigidBodyDynamics)

set(SRC
 prescribedvelocity.cpp
)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
)

set(LIBS
    ${Boost_system_LIBRARY}
)

set(OF_VERSIONS OFesi2112)
setup_lib_target_OF(${PRJ}
    "${SRC}"
    "${OF_INCLUDE_DIRS}" "${OF_LIBS}"
    "${INCLUDE_DIRS}" "${LIBS}"
    "")
linkToolkit_OF_ShLib( ${PRJ} )
