# Try to find OpenFOAM-dev
# Once done this will define
#
# OFesi2112_FOUND          - system has OpenFOAM-dev installed
#
# Note: OFesi2112

include(OpenFOAMfuncs)

#FIND_PATH(OFesi2112_DIR NAMES etc/bashrc
FIND_FILE(OFesi2112_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-v2112
  /opt/OpenFOAM/OpenFOAM-v2112
)

message(STATUS "Found OpenFOAM-v2112 installation: " ${OFesi2112_BASHRC})

SET(OFesi2112_FOUND FALSE)

IF(OFesi2112_BASHRC)

  GET_FILENAME_COMPONENT(OFesi2112_ETC_DIR ${OFesi2112_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OFesi2112_DIR ${OFesi2112_ETC_DIR} PATH)

  detectEnvVars(OFesi2112 WM_PROJECT WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(OFesi2112 LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OFesi2112 LINKEXE LINKEXE_full)
  detectEnvVar(OFesi2112 FOAM_MPI MPI)
  detectEnvVar(OFesi2112 c++FLAGS CXX_FLAGS)

  filterWarningFlags(OFesi2112_CXX_FLAGS)
  set(OFesi2112_CXX_FLAGS "${OFesi2112_CXX_FLAGS} -DOFesi2112 -DOF_VERSION=060505 -DOF_FORK_esi")
  set(OFesi2112_LIBSRC_DIR "${OFesi2112_DIR}/src")
  set(OFesi2112_LIB_DIR "${OFesi2112_DIR}/platforms/${OFesi2112_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" OFesi2112_LINKLIBSO ${OFesi2112_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OFesi2112_LINKEXE ${OFesi2112_LINKEXE_full})

  detectIncPaths(OFesi2112)

  setOFlibvar(OFesi2112
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
regionFaModels
faOptions
)

  detectDepLib(OFesi2112 "${OFesi2112_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OFesi2112 "${OFesi2112_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  set(OFesi2112_INSIGHT_INSTALL_BIN "bin/${OFesi2112_WM_PROJECT}-${OFesi2112_WM_PROJECT_VERSION}")
  set(OFesi2112_INSIGHT_INSTALL_LIB "lib/${OFesi2112_WM_PROJECT}-${OFesi2112_WM_PROJECT_VERSION}")
  set(OFesi2112_INSIGHT_BIN "${CMAKE_BINARY_DIR}/${OFesi2112_INSIGHT_INSTALL_BIN}")
  set(OFesi2112_INSIGHT_LIB "${CMAKE_BINARY_DIR}/${OFesi2112_INSIGHT_INSTALL_LIB}")

  addOFConfig(OFesi2112 of2112 655)

  
  
  macro (setup_exe_target_OFesi2112 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
        
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFesi2112_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFesi2112_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFesi2112_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OFesi2112_INSIGHT_BIN})
    set_target_properties(${targetname} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    target_link_libraries(${targetname}
      ${OFesi2112_LIBRARIES}
      ${ARGN}
      ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OFesi2112_INSIGHT_INSTALL_BIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  
  
  
  macro (setup_lib_target_OFesi2112 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    SET(LIB_SEARCHFLAGS "-L${OFesi2112_LIB_DIR} -L${OFesi2112_LIB_DIR}/${OFesi2112_MPI} -L${OFesi2112_FOAM_EXT_LIBBIN} -L${OFesi2112_SCOTCH_ROOT}/lib")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFesi2112_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFesi2112_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFesi2112_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OFesi2112_INSIGHT_LIB})
    set_target_properties(${targetname} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    target_link_libraries(${targetname} ${OFesi2112_LIBRARIES} ${ARGN})
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OFesi2112_INSIGHT_INSTALL_LIB} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  
  
  
  SET(OFesi2112_FOUND TRUE)
  
ENDIF(OFesi2112_BASHRC)

