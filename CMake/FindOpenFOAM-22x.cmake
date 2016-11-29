# Try to find OpenFOAM-2.2.x
# Once done this will define
#
# OF22x_FOUND          - system has OpenFOAM-2.2.x installed

include(OpenFOAMfuncs)

#FIND_PATH(OF22x_DIR NAMES etc/bashrc
FIND_FILE(OF22x_BASHRC NAMES bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2.x/etc
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2.0/etc
  /opt/OpenFOAM/OpenFOAM-2.2.x/etc
  /opt/OpenFOAM/OpenFOAM-2.2.0/etc
)
message(STATUS "Found OpenFOAM 2.2.x installation: " ${OF22x_BASHRC})

SET(OF22x_FOUND FALSE)

IF(OF22x_BASHRC)
  #set(OF22x_BASHRC "${OF22x_DIR}/etc/bashrc")
  GET_FILENAME_COMPONENT(OF22x_ETC_DIR ${OF22x_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OF22x_DIR ${OF22x_ETC_DIR} PATH)

  detectEnvVars(OF22x WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)

  detectEnvVar(OF22x c++FLAGS CXX_FLAGS)
  set(OF22x_CXX_FLAGS "${OF22x_CXX_FLAGS} -DOF22x")

  detectIncPaths(OF22x)

  set(OF22x_LIBSRC_DIR "${OF22x_DIR}/src")
  set(OF22x_LIB_DIR "${OF22x_DIR}/platforms/${OF22x_WM_OPTIONS}/lib")
  
  detectEnvVar(OF22x LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OF22x LINKEXE LINKEXE_full)
  string(REGEX REPLACE "^[^ ]+" "" OF22x_LINKLIBSO ${OF22x_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF22x_LINKEXE ${OF22x_LINKEXE_full})
  
  detectEnvVar(OF22x FOAM_MPI MPI)

  set(OF22x_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OF22x_WM_PROJECT_VERSION}")
  set(OF22x_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OF22x_WM_PROJECT_VERSION}")

  setOFlibvar(OF22x 
FVFunctionObjects
IOFunctionObjects
SloanRenumber
autoMesh
barotropicCompressibilityModel
blockMesh
cloudFunctionObjects
coalCombustion
combustionModels
compressibleLESModels
compressibleTransportModels
#compressibleTurbulenceModels
decompose
distributed
dsmc
engine
foamCalcFunctions
genericPatchFields
immiscibleIncompressibleTwoPhaseMixture
#incompressibleTurbulenceModels
jobControl
lagrangianSpray
#lagrangianTurbulence
#lagrangianTurbulentSubModels
laminarFlameSpeedModels
molecularMeasurements
molecule
pairPatchAgglomeration
pyrolysisModels
randomProcesses
reconstruct
regionCoupled
regionCoupling
scotchDecomp
sixDoFRigidBodyMotion
solidParticle
solidSpecie
surfaceFilmDerivedFvPatchFields
surfaceFilmModels
systemCall
thermalBaffleModels
topoChangerFvMesh
#turbulenceDerivedFvPatchFields
twoPhaseProperties
utilityFunctionObjects
renumberMethods
edgeMesh
fvMotionSolvers
interfaceProperties
incompressibleTransportModels
lagrangianIntermediate
potential
solidChemistryModel
forces
compressibleRASModels
regionModels
dynamicFvMesh
fvOptions
decompositionMethods
twoPhaseMixture
SLGThermo
radiationModels
distributionModels
solidThermo
chemistryModel
#compressibleTurbulenceModel
liquidMixtureProperties
solidMixtureProperties
ODE
reactionThermophysicalModels
liquidProperties
solidProperties
fluidThermophysicalModels
thermophysicalFunctions
specie
#LEMOS-2.3.x
fieldFunctionObjects
incompressibleLESModels
#incompressibleRASModels
dynamicMesh
sampling
#LESdeltas
#turbulenceModels
#LESfilters
#incompressibleTurbulenceModel
extrudeModel
lagrangian
conversion
finiteVolume
meshTools
triSurface
surfMesh
fileFormats
OpenFOAM
)

  detectDepLib(OF22x "${OF22x_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OF22x "${OF22x_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  list(APPEND INSIGHT_OFES_VARCONTENT "OF22x@`find \\\${PATH//:/ } -maxdepth 1 -name insight.bashrc.of22x -print -quit`#220")
  set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
alias of22x=\"source insight.bashrc.of22x\"
")
  create_script("insight.bashrc.of22x"
"source ${OF22x_BASHRC}

foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${OF22x_INSIGHT_LIB}
#- Clean LD_LIBRARY_PATH
cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
export PATH=$PATH:${OF22x_INSIGHT_BIN}
#- Clean PATH
cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
")

  macro (setup_exe_target_OF22x targetname sources exename includes)
    #message(STATUS "target " ${targetname} ": includes=" ${includes})
    get_directory_property(temp LINK_DIRECTORIES)
    
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF22x_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF22x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF22x_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OF22x_INSIGHT_BIN})
    target_link_libraries(${targetname} ${OF22x_LIBRARIES} ${ARGN} ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF22x_FOAM_APPBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  macro (setup_lib_target_OF22x targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    SET(LIB_SEARCHFLAGS "-L${OF22x_LIB_DIR} -L${OF22x_LIB_DIR}/${OF22x_MPI} -L${OF22x_FOAM_EXT_LIBBIN} -L${OF22x_SCOTCH_ROOT}/lib")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF22x_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF22x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF22x_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OF22x_INSIGHT_LIB})
    target_link_libraries(${targetname} ${OF22x_LIBRARIES} ${ARGN}) 
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF22x_FOAM_LIBBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(OF22x_FOUND TRUE)
  
ENDIF(OF22x_BASHRC)

