set(PRJ mapFields22)

set(SRC mapFields.C mapLagrangian.C calculateMeshToMeshAddressing.C calculateMeshToMeshWeights.C meshToMesh.C tetOverlapVolume.C 
)

set(OF_INCLUDE_DIRS
  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
  meshTools lagrangian lagrangian/basic
)

set(OF_LIBS 
  finiteVolume meshTools lagrangian
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
)

set(OF_VERSIONS OF22x OF22eng OF23x)
setup_exe_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "" "")
