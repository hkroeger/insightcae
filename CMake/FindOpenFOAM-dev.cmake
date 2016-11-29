# Try to find OpenFOAM-dev
# Once done this will define
#
# OFdev_FOUND          - system has OpenFOAM-dev installed

include(OpenFOAMfuncs)

#FIND_PATH(OFdev_DIR NAMES etc/bashrc
FIND_FILE(OFdev_BASHRC NAMES bashrc
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-dev/etc
  /opt/OpenFOAM/OpenFOAM-dev/etc
)

message(STATUS "Found OpenFOAM-dev installation: " ${OFdev_BASHRC})

SET(OFdev_FOUND FALSE)

IF(OFdev_BASHRC)

  GET_FILENAME_COMPONENT(OFdev_ETC_DIR ${OFdev_BASHRC} PATH)
  GET_FILENAME_COMPONENT(OFdev_DIR ${OFdev_ETC_DIR} PATH)

  detectEnvVars(OFdev WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(OFdev LINKLIBSO LINKLIBSO_full)
  detectEnvVar(OFdev LINKEXE LINKEXE_full)
  detectEnvVar(OFdev FOAM_MPI MPI)
  detectEnvVar(OFdev c++FLAGS CXX_FLAGS)

  set(OFdev_CXX_FLAGS "${OFdev_CXX_FLAGS} -DOFdev")
  set(OFdev_LIBSRC_DIR "${OFdev_DIR}/src")
  set(OFdev_LIB_DIR "${OFdev_DIR}/platforms/${OFdev_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" OFdev_LINKLIBSO ${OFdev_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" OFdev_LINKEXE ${OFdev_LINKEXE_full})

  detectIncPaths(OFdev)

  setOFlibvar(OFdev 
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
#incompressibleTurbulenceModel
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

  detectDepLib(OFdev "${OFdev_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(OFdev "${OFdev_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")

  set(OFdev_INSIGHT_BIN "${CMAKE_BINARY_DIR}/bin/OpenFOAM-${OFdev_WM_PROJECT_VERSION}")
  set(OFdev_INSIGHT_LIB "${CMAKE_BINARY_DIR}/lib/OpenFOAM-${OFdev_WM_PROJECT_VERSION}")

  addOFConfig(OFdev ofdev 400)
#   list(APPEND INSIGHT_OFES_VARCONTENT "OF23x@`find \\\${PATH//:/ } -maxdepth 1 -name insight.bashrc.of23x -print -quit`#230")
#   set(INSIGHT_OF_ALIASES "${INSIGHT_OF_ALIASES}
# alias of23x=\"source insight.bashrc.of23x\"
# ")
#   create_script("insight.bashrc.of23x"
# "source ${OFdev_BASHRC}
# 
# foamClean=$WM_PROJECT_DIR/bin/foamCleanPath
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${OFdev_INSIGHT_LIB}
# #- Clean LD_LIBRARY_PATH
# cleaned=`$foamClean \"$LD_LIBRARY_PATH\"` && LD_LIBRARY_PATH=\"$cleaned\"
# export PATH=$PATH:${OFdev_INSIGHT_BIN}
# #- Clean PATH
# cleaned=`$foamClean \"$PATH\"` && PATH=\"$cleaned\"
# ")




  macro (setup_exe_target_OFdev targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
        
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFdev_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFdev_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFdev_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OFdev_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${OFdev_LIBRARIES}
      ${ARGN}
      ) 
    install(TARGETS ${targetname} RUNTIME DESTINATION ${OFdev_FOAM_APPBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})

    set_directory_properties(LINK_DIRECTORIES ${temp})
    get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  
  
  
  macro (setup_lib_target_OFdev targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)

    SET(LIB_SEARCHFLAGS "-L${OFdev_LIB_DIR} -L${OFdev_LIB_DIR}/${OFdev_MPI} -L${OFdev_FOAM_EXT_LIBBIN} -L${OFdev_SCOTCH_ROOT}/lib")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${OFdev_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${OFdev_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${OFdev_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OFdev_INSIGHT_LIB})
    target_link_libraries(${targetname} ${OFdev_LIBRARIES} ${ARGN}) 
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
      )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${OFdev_FOAM_LIBBIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  
  
  
  SET(OFdev_FOUND TRUE)
  
ENDIF(OFdev_BASHRC)
