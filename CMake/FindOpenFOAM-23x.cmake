# Try to find OpenFOAM-2.3.x
# Once done this will define
#
# OF23x_FOUND          - system has OpenFOAM-2.3.x installed

include(OpenFOAMfuncs)

#FIND_PATH(OF23x_DIR NAMES etc/bashrc
FIND_FILE(OF23x_BASHRC NAMES bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.3.x/etc
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.3.0/etc
  /opt/OpenFOAM/OpenFOAM-2.3.x/etc
  /opt/OpenFOAM/OpenFOAM-2.3.0/etc
  /opt/openfoam230/etc
)

message(STATUS "Found OpenFOAM 2.3.x installation: " ${OF23x_BASHRC})

SET(OF23x_FOUND FALSE)

IF(OF23x_BASHRC)

  GET_FILENAME_COMPONENT(OF23x_ETC_DIR ${OF23x_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OF23x_DIR ${OF23x_ETC_DIR} PATH)

  detectEnvVars(OF23x WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(OF23x LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OF23x LINKEXE LINKEXE_full)
  detectEnvVar(OF23x FOAM_MPI MPI)
  detectEnvVar(OF23x c++FLAGS CXX_FLAGS)

  set(OF23x_CXX_FLAGS "${OF23x_CXX_FLAGS} -DOF23x")
  set(OF23x_LIBSRC_DIR "${OF23x_DIR}/src")
  set(OF23x_LIB_DIR "${OF23x_DIR}/platforms/${OF23x_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" OF23x_LINKLIBSO ${OF23x_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OF23x_LINKEXE ${OF23x_LINKEXE_full})

  detectIncPaths(OF23x)

  setOFlibvar(OF23x 
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
lagrangianTurbulentSubModels
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


# FVFunctionObjects
# IOFunctionObjects
# SloanRenumber
# autoMesh
# barotropicCompressibilityModel
# blockMesh
# cloudFunctionObjects
# coalCombustion
# combustionModels
# compressibleLESModels
# compressibleTransportModels
# #compressibleTurbulenceModels
# decompose
# distributed
# dsmc
# engine
# foamCalcFunctions
# genericPatchFields
# immiscibleIncompressibleTwoPhaseMixture
# #incompressibleTurbulenceModels
# jobControl
# lagrangianSpray
# #lagrangianTurbulence
# #lagrangianTurbulentSubModels
# laminarFlameSpeedModels
# molecularMeasurements
# molecule
# pairPatchAgglomeration
# pyrolysisModels
# randomProcesses
# reconstruct
# regionCoupled
# regionCoupling
# scotchDecomp
# sixDoFRigidBodyMotion
# solidParticle
# solidSpecie
# surfaceFilmDerivedFvPatchFields
# surfaceFilmModels
# systemCall
# thermalBaffleModels
# topoChangerFvMesh
# #turbulenceDerivedFvPatchFields
# twoPhaseProperties
# utilityFunctionObjects
# renumberMethods
# edgeMesh
# fvMotionSolvers
# interfaceProperties
# incompressibleTransportModels
# lagrangianIntermediate
# potential
# solidChemistryModel
# forces
# compressibleRASModels
# regionModels
# dynamicFvMesh
# fvOptions
# decompositionMethods
# twoPhaseMixture
# SLGThermo
# radiationModels
# distributionModels
# solidThermo
# chemistryModel
# #compressibleTurbulenceModel
# liquidMixtureProperties
# solidMixtureProperties
# ODE
# reactionThermophysicalModels
# liquidProperties
# solidProperties
# fluidThermophysicalModels
# thermophysicalFunctions
# specie
# #LEMOS-2.3.x
# fieldFunctionObjects
# incompressibleLESModels
# #incompressibleRASModels
# dynamicMesh
# sampling
# #LESdeltas
# #turbulenceModels
# #LESfilters
# #incompressibleTurbulenceModel
# extrudeModel
# lagrangian
# conversion
# finiteVolume
# meshTools
# triSurface
# surfMesh
# fileFormats
# OpenFOAM
)

  detectDepLib(OF23x "${OF23x_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OF23x "${OF23x_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  set(OF23x_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OF23x_WM_PROJECT_VERSION}")
  set(OF23x_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OF23x_WM_PROJECT_VERSION}")

  addOFConfig(OF23x of23x 230)
#   list(APPEND INSIGHT_OFES_VARCONTENT "OF23x@`find \\\${PATH//:/ } -maxdepth 1 -name insight.bashrc.of23x -print -quit`#230")
#   set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
# alias of23x=\"source insight.bashrc.of23x\"
# ")
#   create_script("insight.bashrc.of23x"
# "source ${OF23x_BASHRC}
# 
# foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${OF23x_INSIGHT_LIB}
# #- Clean LD_LIBRARY_PATH
# cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
# export PATH=$PATH:${OF23x_INSIGHT_BIN}
# #- Clean PATH
# cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
# ")




  macro (setup_exe_target_OF23x targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
        
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF23x_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF23x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF23x_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OF23x_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${OF23x_LIBRARIES}
      ${ARGN}
      ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OF23x_FOAM_APPBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  
  
  
  macro (setup_lib_target_OF23x targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    SET(LIB_SEARCHFLAGS "-L${OF23x_LIB_DIR} -L${OF23x_LIB_DIR}/${OF23x_MPI} -L${OF23x_FOAM_EXT_LIBBIN} -L${OF23x_SCOTCH_ROOT}/lib")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OF23x_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OF23x_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OF23x_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OF23x_INSIGHT_LIB})
    target_link_libraries(${targetname} ${OF23x_LIBRARIES} ${ARGN}) 
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OF23x_FOAM_LIBBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  
  
  
  SET(OF23x_FOUND TRUE)
  
ENDIF(OF23x_BASHRC)

