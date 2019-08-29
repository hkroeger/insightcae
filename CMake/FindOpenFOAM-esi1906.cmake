# Try to find OpenFOAM-esi1906
# Once done this will define
#
# OFesi1906_FOUND          - system has OpenFOAM-dev installed
#
# Note: OFesi1906

include(OpenFOAMfuncs)

FIND_FILE(OFesi1906_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-v1906
  /opt/OpenFOAM/OpenFOAM-v1906
)

message(STATUS "Found OpenFOAM-v1906 installation: " ${OFesi1906_BASHRC})

SET(OFesi1906_FOUND FALSE)

IF(OFesi1906_BASHRC)

  GET_FILENAME_COMPONENT(OFesi1906_ETC_DIR ${OFesi1906_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OFesi1906_DIR ${OFesi1906_ETC_DIR} PATH)

  detectEnvVars(OFesi1906 WM_PROJECT WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(OFesi1906 LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OFesi1906 LINKEXE LINKEXE_full)
  detectEnvVar(OFesi1906 FOAM_MPI MPI)
  detectEnvVar(OFesi1906 c++FLAGS CXX_FLAGS)

  set(OFesi1906_CXX_FLAGS "${OFesi1906_CXX_FLAGS} -DOFesi1906 -DOF_VERSION=060500 -DOF_FORK_esi")
  set(OFesi1906_LIBSRC_DIR "${OFesi1906_DIR}/src")
  set(OFesi1906_LIB_DIR "${OFesi1906_DIR}/platforms/${OFesi1906_WM_OPTIONS}/lib")

  string(REGEX REPLACE "^[^ ]+" "" OFesi1906_LINKLIBSO ${OFesi1906_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OFesi1906_LINKEXE ${OFesi1906_LINKEXE_full})

  detectIncPaths(OFesi1906)

  setOFlibvar(OFesi1906
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

  detectDepLib(OFesi1906 "${OFesi1906_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OFesi1906 "${OFesi1906_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  set(OFesi1906_INSIGHT_INSTALL_BIN "bin/${OFesi1906_WM_PROJECT}-${OFesi1906_WM_PROJECT_VERSION}")
  set(OFesi1906_INSIGHT_INSTALL_LIB "lib/${OFesi1906_WM_PROJECT}-${OFesi1906_WM_PROJECT_VERSION}")
  set(OFesi1906_INSIGHT_BIN "${CMAKE_BINARY_DIR}/${OFesi1906_INSIGHT_INSTALL_BIN}")
  set(OFesi1906_INSIGHT_LIB "${CMAKE_BINARY_DIR}/${OFesi1906_INSIGHT_INSTALL_LIB}")

  addOFConfig(OFesi1906 of1906 610)



  macro (setup_exe_target_OFesi1906 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    add_executable(${targetname} ${sources})

    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFesi1906_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFesi1906_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFesi1906_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OFesi1906_INSIGHT_BIN})
    target_link_libraries(${targetname}
      ${OFesi1906_LIBRARIES}
      ${ARGN}
      )
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OFesi1906_INSIGHT_INSTALL_BIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()




  macro (setup_lib_target_OFesi1906 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    SET(LIB_SEARCHFLAGS "-L${OFesi1906_LIB_DIR} -L${OFesi1906_LIB_DIR}/${OFesi1906_MPI} -L${OFesi1906_FOAM_EXT_LIBBIN} -L${OFesi1906_SCOTCH_ROOT}/lib")

    add_library(${targetname} SHARED ${sources})

    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFesi1906_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFesi1906_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFesi1906_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OFesi1906_INSIGHT_LIB})
    target_link_libraries(${targetname} ${OFesi1906_LIBRARIES} ${ARGN})
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OFesi1906_INSIGHT_INSTALL_LIB} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()




  SET(OFesi1906_FOUND TRUE)

ENDIF(OFesi1906_BASHRC)

