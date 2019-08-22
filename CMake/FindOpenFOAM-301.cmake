# Try to find OpenFOAM-3.0.1
# Once done this will define
#
# OF301_FOUND          - system has OpenFOAM-3.0.1 installed

include(OpenFOAMfuncs)

#FIND_PATH(OF301_DIR NAMES etc/bashrc
FIND_FILE(OF301_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-3.0.1
  /opt/OpenFOAM/OpenFOAM-3.0.1
)

message(STATUS "Found OpenFOAM-3.0.1 installation: " ${OF301_BASHRC})

SET(OF301_FOUND FALSE)

IF(OF301_BASHRC)

  GET_FILENAME_COMPONENT(OF301_ETC_DIR ${OF301_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OF301_DIR ${OF301_ETC_DIR} PATH)

  detectEnvVars(OF301 WM_PROJECT WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(OF301 LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OF301 LINKEXE LINKEXE_full)
  detectEnvVar(OF301 FOAM_MPI MPI)
  detectEnvVar(OF301 c++FLAGS CXX_FLAGS)

  set(OF301_CXX_FLAGS "${OF301_CXX_FLAGS} -DOF301")
  set(OF301_LIBSRC_DIR "${OF301_DIR}/src")
  set(OF301_LIB_DIR "${OF301_DIR}/platforms/${OF301_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" OF301_LINKLIBSO ${OF301_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF301_LINKEXE ${OF301_LINKEXE_full})

  detectIncPaths(OF301)

  setOFlibvar(OF301 
incompressibleTurbulenceModels
turbulenceModels
turbulenceModelSchemes
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
liquidPropertiesFvPatchFields
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
sampling
FVFunctionObjects
jobControl
systemCall
utilityFunctionObjects
forces
fieldFunctionObjects
IOFunctionObjects
cloudFunctionObjects
foamCalcFunctions
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
)

  detectDepLib(OF301 "${OF301_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OF301 "${OF301_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  set(OF301_INSIGHT_INSTALL_BIN "bin/${OF301_WM_PROJECT}-${OF301_WM_PROJECT_VERSION}")
  set(OF301_INSIGHT_INSTALL_LIB "lib/${OF301_WM_PROJECT}-${OF301_WM_PROJECT_VERSION}")
  set(OF301_INSIGHT_BIN "${CMAKE_BINARY_DIR}/${OF301_INSIGHT_INSTALL_BIN}")
  set(OF301_INSIGHT_LIB "${CMAKE_BINARY_DIR}/${OF301_INSIGHT_INSTALL_LIB}")

  addOFConfig(OF301 of301 301)




  macro (setup_exe_target_OF301 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
        
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF301_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF301_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF301_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OF301_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${OF301_LIBRARIES}
      ${ARGN}
      ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF301_INSIGHT_INSTALL_BIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  
  
  
  macro (setup_lib_target_OF301 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    SET(LIB_SEARCHFLAGS "-L${OF301_LIB_DIR} -L${OF301_LIB_DIR}/${OF301_MPI} -L${OF301_FOAM_EXT_LIBBIN} -L${OF301_SCOTCH_ROOT}/lib")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF301_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF301_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF301_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OF301_INSIGHT_LIB})
    target_link_libraries(${targetname} ${OF301_LIBRARIES} ${ARGN}) 
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF301_INSIGHT_INSTALL_LIB} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  
  
  
  SET(OF301_FOUND TRUE)
  
ENDIF(OF301_BASHRC)

