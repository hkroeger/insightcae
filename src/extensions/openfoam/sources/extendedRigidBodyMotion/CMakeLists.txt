
set(PRJ extendedRigidBodyMotion)

set(SRC 
 extendedRigidBodyMotion.C
 additionalforceandmoment.cpp
 directforce.cpp
 patchmomentumforce.cpp
)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS 
)

set(INCLUDE_DIRS 
    ${Boost_INCLUDE_DIR}
)

set(LIBS
    ${Boost_system_LIBRARY}
)

set(OF_VERSIONS OFplus OFesi1806 OFesi1906 OFesi2112)
setup_lib_target_OF(${PRJ}
    "${SRC}"
    "${OF_INCLUDE_DIRS}" "${OF_LIBS}"
    "${INCLUDE_DIRS}" "${LIBS}"
    "extendedForcesFunctionObject;forceSources;globalCoordinateSystems")

