set(SRC projectsplineedge.cpp)

set(OF_INCLUDE_DIRS
)

set(OF_LIBS
    blockMesh
)

set(INCLUDE_DIRS
  ${insight_INCLUDE_DIR}
)

set(OF_VERSIONS OFesi2112)
setup_lib_target_OF(blockMeshProjectedSplineEdge "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")

