set(SRC surfaceProjectLine.C)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
#  turbulenceModels/incompressible/LES turbulenceModels/LES/LESfilters
#  meshTools sampling surfMesh triSurface
)

set(OF_LIBS
# finiteVolume LESfilters sampling dynamicMesh triSurface
)

set(INCLUDE_DIRS 
  ${insight_INCLUDE_DIR}
)

#set(OF_VERSIONS OF23x)
setup_exe_target_OF(surfaceProjectLine "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "uniof" "")
