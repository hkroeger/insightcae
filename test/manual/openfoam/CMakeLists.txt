SET(OF_VERSIONS OFesi1806 OFesi2112)
setup_exe_target_OF(test_extendedRigidBodyMotion
    "extendedRigidBodyMotion.cpp" "" "" "${Boost_INCLUDE_DIR}" "uniof" "")
