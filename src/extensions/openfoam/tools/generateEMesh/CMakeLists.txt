set(SRC generateemesh.cpp)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

setup_exe_target_OF(generateEMesh "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof;insightcad" "")
linkToolkit_OF_Exe( generateEMesh )
