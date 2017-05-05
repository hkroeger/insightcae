# Try to find OpenFOAM-plus
# Once done this will define
#
# OFplus_FOUND          - system has OpenFOAM-plus installed

include(OpenFOAMfuncs)

#FIND_PATH(OFplus_DIR NAMES etc/bashrc
FIND_FILE(OFplus_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-plus
  /opt/OpenFOAM/OpenFOAM-plus
)

message(STATUS "Found OpenFOAM-plus installation: " ${OFplus_BASHRC})

SET(OFplus_FOUND FALSE)

IF(OFplus_BASHRC)

  GET_FILENAME_COMPONENT(OFplus_ETC_DIR ${OFplus_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OFplus_DIR ${OFplus_ETC_DIR} PATH)

  detectEnvVars(OFplus WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(OFplus LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OFplus LINKEXE LINKEXE_full)
  detectEnvVar(OFplus FOAM_MPI MPI)
  detectEnvVar(OFplus c++FLAGS CXX_FLAGS)

  set(OFplus_CXX_FLAGS "${OFplus_CXX_FLAGS} -DOFplus")
  set(OFplus_LIBSRC_DIR "${OFplus_DIR}/src")
  set(OFplus_LIB_DIR "${OFplus_DIR}/platforms/${OFplus_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" OFplus_LINKLIBSO ${OFplus_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OFplus_LINKEXE ${OFplus_LINKEXE_full})

  detectIncPaths(OFplus)

  setOFlibvar(OFplus 
# incompressibleTurbulenceModels
# turbulenceModels
# turbulenceModelSchemes
# compressibleTurbulenceModels
# sixDoFRigidBodyMotion
# engine
# SLGThermo
# fluidThermophysicalModels
# laminarFlameSpeedModels
# solidThermo
# radiationModels
# liquidProperties
# solidMixtureProperties
# liquidMixtureProperties
# liquidPropertiesFvPatchFields
# solidProperties
# barotropicCompressibilityModel
# solidSpecie
# specie
# reactionThermophysicalModels
# thermophysicalFunctions
# chemistryModel
# solidChemistryModel
# pairPatchAgglomeration
# reconstruct
# decompositionMethods
# decompose
# scotchDecomp
# distributed
# topoChangerFvMesh
# edgeMesh
# blockMesh
# extrudeModel
# snappyHexMesh
# regionCoupled
# pyrolysisModels
# regionModels
# regionCoupling
# surfaceFilmDerivedFvPatchFields
# surfaceFilmModels
# thermalBaffleModels
# meshTools
# triSurface
# finiteVolume
# dynamicMesh
# fvMotionSolvers
# dynamicFvMesh
# coalCombustion
# molecule
# potential
# molecularMeasurements
# lagrangian
# solidParticle
# lagrangianTurbulence
# lagrangianSpray
# DSMC
# lagrangianIntermediate
# distributionModels
# rigidBodyMeshMotion
# conversion
# sampling
# FVFunctionObjects
# jobControl
# systemCall
# utilityFunctionObjects
# forces
# fieldFunctionObjects
# IOFunctionObjects
# cloudFunctionObjects
# foamCalcFunctions
# surfMesh
# rigidBodyDynamics
# ODE
# incompressibleTransportModels
# twoPhaseMixture
# interfaceProperties
# twoPhaseProperties
# immiscibleIncompressibleTwoPhaseMixture
# compressibleTransportModels
# combustionModels
# fvOptions
# OpenFOAM
# renumberMethods
# fileFormats
# genericPatchFields
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
)

  detectDepLib(OFplus "${OFplus_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OFplus "${OFplus_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  set(OFplus_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OFplus_WM_PROJECT_VERSION}")
  set(OFplus_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OFplus_WM_PROJECT_VERSION}")

  addOFConfig(OFplus ofplus 302)
#   list(APPEND INSIGHT_OFES_VARCONTENT "OF23x@`find \\\${PATH//:/ } -maxdepth 1 -name insight.bashrc.of23x -print -quit`#230")
#   set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
# alias of23x=\"source insight.bashrc.of23x\"
# ")
#   create_script("insight.bashrc.of23x"
# "source ${OFplus_BASHRC}
# 
# foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${OFplus_INSIGHT_LIB}
# #- Clean LD_LIBRARY_PATH
# cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
# export PATH=$PATH:${OFplus_INSIGHT_BIN}
# #- Clean PATH
# cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
# ")




  macro (setup_exe_target_OFplus targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
        
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFplus_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFplus_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFplus_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OFplus_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${OFplus_LIBRARIES}
      ${ARGN}
      ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OFplus_FOAM_APPBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  
  
  
  macro (setup_lib_target_OFplus targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    SET(LIB_SEARCHFLAGS "-L${OFplus_LIB_DIR} -L${OFplus_LIB_DIR}/${OFplus_MPI} -L${OFplus_FOAM_EXT_LIBBIN} -L${OFplus_SCOTCH_ROOT}/lib")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFplus_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFplus_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFplus_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OFplus_INSIGHT_LIB})
    target_link_libraries(${targetname} ${OFplus_LIBRARIES} ${ARGN}) 
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OFplus_FOAM_LIBBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  
  
  
  SET(OFplus_FOUND TRUE)
  
ENDIF(OFplus_BASHRC)

