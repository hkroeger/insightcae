

set(PRJ fixedMeanValueBC)

set(SRC 
 timeVaryingFixedMeanValueFvPatchFields.C
)

set(OF_INCLUDE_DIRS
#  OpenFOAM finiteVolume OSspecific/POSIX lduSolvers 
#  meshTools triSurface sampling surfMesh
)

set(OF_LIBS 
#finiteVolume 
)

set(INCLUDE_DIRS 
)

set(LIBS 
)

set(OF_VERSIONS OF21x OF22x OF22eng OF23x OFplus Fx32 Fx41) # exists already in OFesi1806 and 1906
setup_lib_target_OF(${PRJ} "${SRC}" "${OF_INCLUDE_DIRS}" "${OF_LIBS}" "${INCLUDE_DIRS}" "${LIBS}" "")
