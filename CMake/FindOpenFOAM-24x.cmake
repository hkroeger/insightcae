# Try to find OpenFOAM-2.4.x
# Once done this will define
#
# OF24x_FOUND          - system has OpenFOAM-2.4.x installed

include(OpenFOAMfuncs)

#FIND_PATH(OF24x_DIR NAMES etc/bashrc
FIND_FILE(OF24x_BASHRC NAMES bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.4.x/etc
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.4.0/etc
  /opt/OpenFOAM/OpenFOAM-2.4.x/etc
  /opt/OpenFOAM/OpenFOAM-2.4.0/etc
  /opt/openfoam240/etc
)

message(STATUS "Found OpenFOAM 2.4.x installation: " ${OF24x_BASHRC})

SET(OF24x_FOUND FALSE)

IF(OF24x_BASHRC)

  GET_FILENAME_COMPONENT(OF24x_ETC_DIR ${OF24x_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OF24x_DIR ${OF24x_ETC_DIR} PATH)

  detectEnvVars(OF24x WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(OF24x LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OF24x LINKEXE LINKEXE_full)
  detectEnvVar(OF24x FOAM_MPI MPI)
  detectEnvVar(OF24x c++FLAGS CXX_FLAGS)

  set(OF24x_CXX_FLAGS "${OF24x_CXX_FLAGS} -DOF23x -DOF24x")
  set(OF24x_LIBSRC_DIR "${OF24x_DIR}/src")
  set(OF24x_LIB_DIR "${OF24x_DIR}/platforms/${OF24x_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" OF24x_LINKLIBSO ${OF24x_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF24x_LINKEXE ${OF24x_LINKEXE_full})

  detectIncPaths(OF24x "src/TurbulenceModels/*") # skip (unfinished?) TurbulenceModels because of conflicting headers

  setOFlibvar(OF24x 
#incompressibleTurbulenceModels
#turbulenceModels
#compressibleTurbulenceModels
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
metisDecomp
decompositionMethods
decompose
scotchDecomp
distributed
topoChangerFvMesh
incompressibleRASModels
incompressibleTurbulenceModel
incompressibleLESModels
turbulenceDerivedFvPatchFields
LESfilters
LESdeltas
compressibleRASModels
compressibleTurbulenceModel
compressibleLESModels
edgeMesh
blockMesh
autoMesh
extrudeModel
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
#lagrangianTurbulentSubModels
molecule
potential
molecularMeasurements
lagrangian
solidParticle
lagrangianTurbulence
lagrangianSpray
lagrangianIntermediate
distributionModels
dsmc
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
SloanRenumber
fileFormats
genericPatchFields
randomProcesses
)

  detectDepLib(OF24x "${OF24x_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OF24x "${OF24x_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  set(OF24x_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OF24x_WM_PROJECT_VERSION}")
  set(OF24x_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OF24x_WM_PROJECT_VERSION}")

  addOFConfig(OF24x of24x 240)


  macro (setup_exe_target_OF24x targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
        
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF24x_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF24x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF24x_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OF24x_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${OF24x_LIBRARIES}
      ${ARGN}
      ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF24x_FOAM_APPBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  
  
  
  macro (setup_lib_target_OF24x targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    SET(LIB_SEARCHFLAGS "-L${OF24x_LIB_DIR} -L${OF24x_LIB_DIR}/${OF24x_MPI} -L${OF24x_FOAM_EXT_LIBBIN} -L${OF24x_SCOTCH_ROOT}/lib")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF24x_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF24x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF24x_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OF24x_INSIGHT_LIB})
    target_link_libraries(${targetname} ${OF24x_LIBRARIES} ${ARGN}) 
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF24x_FOAM_LIBBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  
  
  
  SET(OF24x_FOUND TRUE)
  
ENDIF(OF24x_BASHRC)

