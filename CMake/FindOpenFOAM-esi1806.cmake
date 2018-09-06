# Try to find OpenFOAM-dev
# Once done this will define
#
# OFesi1806_FOUND          - system has OpenFOAM-dev installed
#
# Note: OFesi1806

include(OpenFOAMfuncs)

#FIND_PATH(OFesi1806_DIR NAMES etc/bashrc
FIND_FILE(OFesi1806_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-v1806
  /opt/OpenFOAM/OpenFOAM-v1806
)

message(STATUS "Found OpenFOAM-v1806 installation: " ${OFesi1806_BASHRC})

SET(OFesi1806_FOUND FALSE)

IF(OFesi1806_BASHRC)

  GET_FILENAME_COMPONENT(OFesi1806_ETC_DIR ${OFesi1806_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OFesi1806_DIR ${OFesi1806_ETC_DIR} PATH)

  detectEnvVars(OFesi1806 WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(OFesi1806 LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OFesi1806 LINKEXE LINKEXE_full)
  detectEnvVar(OFesi1806 FOAM_MPI MPI)
  detectEnvVar(OFesi1806 c++FLAGS CXX_FLAGS)

  set(OFesi1806_CXX_FLAGS "${OFesi1806_CXX_FLAGS} -DOFesi1806")
  set(OFesi1806_LIBSRC_DIR "${OFesi1806_DIR}/src")
  set(OFesi1806_LIB_DIR "${OFesi1806_DIR}/platforms/${OFesi1806_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" OFesi1806_LINKLIBSO ${OFesi1806_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OFesi1806_LINKEXE ${OFesi1806_LINKEXE_full})

  detectIncPaths(OFesi1806)

  setOFlibvar(OFesi1806
incompressibleTurbulenceModels
turbulenceModels
compressibleTurbulenceModels
sixDoFRigidBodyMotion
engine
SLGThermo
fluidThermophysicalModels
laminarFlameSpeedModels
solidThermo
radiationModels
liquidProperties
solidMixtureProperties
liquidMixtureProperties
thermophysicalProperties
solidProperties
barotropicCompressibilityModel
solidSpecie
specie
reactionThermophysicalModels
thermophysicalFunctions
chemistryModel
solidChemistryModel
pairPatchAgglomeration
reconstruct
decompositionMethods
decompose
scotchDecomp
distributed
topoChangerFvMesh
edgeMesh
blockMesh
extrudeModel
snappyHexMesh
regionCoupled
pyrolysisModels
regionModels
regionCoupling
surfaceFilmDerivedFvPatchFields
surfaceFilmModels
thermalBaffleModels
meshTools
triSurface
finiteVolume
finiteArea
dynamicMesh
fvMotionSolvers
dynamicFvMesh
coalCombustion
molecule
potential
molecularMeasurements
lagrangian
solidParticle
lagrangianTurbulence
lagrangianSpray
DSMC
lagrangianIntermediate
distributionModels
rigidBodyMeshMotion
conversion
utilityFunctionObjects
forces
fieldFunctionObjects
solverFunctionObjects
lagrangianFunctionObjects
sampling
surfMesh
rigidBodyDynamics
ODE
incompressibleTransportModels
twoPhaseMixture
interfaceProperties
twoPhaseProperties
immiscibleIncompressibleTwoPhaseMixture
compressibleTransportModels
combustionModels
fvOptions
OpenFOAM
renumberMethods
fileFormats
genericPatchFields
randomProcesses
atmosphericModels
overset
)

  detectDepLib(OFesi1806 "${OFesi1806_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OFesi1806 "${OFesi1806_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  set(OFesi1806_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OFesi1806_WM_PROJECT_VERSION}")
  set(OFesi1806_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OFesi1806_WM_PROJECT_VERSION}")

  addOFConfig(OFesi1806 of1806 600)

  
  
  macro (setup_exe_target_OFesi1806 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
        
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFesi1806_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFesi1806_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFesi1806_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OFesi1806_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${OFesi1806_LIBRARIES}
      ${ARGN}
      ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OFesi1806_FOAM_APPBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  
  
  
  macro (setup_lib_target_OFesi1806 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    SET(LIB_SEARCHFLAGS "-L${OFesi1806_LIB_DIR} -L${OFesi1806_LIB_DIR}/${OFesi1806_MPI} -L${OFesi1806_FOAM_EXT_LIBBIN} -L${OFesi1806_SCOTCH_ROOT}/lib")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFesi1806_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFesi1806_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFesi1806_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OFesi1806_INSIGHT_LIB})
    target_link_libraries(${targetname} ${OFesi1806_LIBRARIES} ${ARGN})
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OFesi1806_FOAM_LIBBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  
  
  
  SET(OFesi1806_FOUND TRUE)
  
ENDIF(OFesi1806_BASHRC)

