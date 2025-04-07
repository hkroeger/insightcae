# Try to find OpenFOAM-1.6-ext
# Once done this will define
#
# Fx41_FOUND          - system has foam-extend-4.1 installed
# Fx41_LIBRARIES      - all OpenFOAM libraries
# Fx41_INCLUDE_PATHS  - all OpenFOAM include paths

include(OpenFOAMfuncs)

FIND_FILE(Fx41_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/foam/foam-extend-4.1
  $ENV{HOME}/OpenFOAM/foam-extend-4.1
  /opt/foam/foam-extend-4.1
  /opt/OpenFOAM/foam-extend-4.1
)

message(STATUS "Found foam-extend-4.1 installation: " ${Fx41_BASHRC})

SET(Fx41_FOUND FALSE)

IF(Fx41_BASHRC)

  GET_FILENAME_COMPONENT(Fx41_ETC_DIR ${Fx41_BASHRC} PATH)
  GET_FILENAME_COMPONENT(Fx41_DIR ${Fx41_ETC_DIR} PATH)

  detectEnvVars(Fx41
    WM_PROJECT
    WM_PROJECT_VERSION
    WM_OPTIONS
    FOAM_EXT_LIBBIN
    SCOTCH_ROOT
    FOAM_APPBIN
    FOAM_LIBBIN
    MESQUITE_LIB_DIR
    MESQUITE_INCLUDE_DIR
    PARMGRIDGEN_LIB_DIR
    PARMGRIDGEN_INCLUDE_DIR
    LINKLIBSO
    LINKEXE
    FOAM_MPI
    c++FLAGS
  )
  filterWarningFlags(Fx41_c++FLAGS)

  set(Fx41_CXX_FLAGS "${Fx41_c++FLAGS} -DFx41 -DOF16ext -DOF_VERSION=010604 -DOF_FORK_extend")
  set(Fx41_LIBSRC_DIR "${Fx41_DIR}/src")
  set(Fx41_LIB_DIR "${Fx41_DIR}/platforms/${Fx41_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" Fx41_LINKLIBSO ${Fx41_LINKLIBSO})
  string(REGEX REPLACE "^[^ ]+" "" Fx41_LINKEXE ${Fx41_LINKEXE})
  
  detectIncPaths(Fx41)

  setOFlibvar(Fx41 
engine
solids
basicThermophysicalModels
laminarFlameSpeedModels
radiation
solidMixture
barotropicCompressibilityModel
liquids
specie
reactionThermophysicalModels
thermophysicalFunctions
chemistryModel
liquidMixture
pdf
MGridGenGAMGAgglomeration
# metisDecomp
decompositionMethods
decomposeReconstruct
# parMetisDecomp
# scotchDecomp
incompressibleRASModels
incompressibleTurbulenceModel
incompressibleLESModels
LESfilters
LESdeltas
compressibleRASModels
compressibleTurbulenceModel
compressibleLESModels
edgeMesh
blockMesh
autoMesh
extrudeModel
cfMesh
meshTools
errorEstimation
dbns
finiteVolume
equationReader
dynamicTopoFvMesh
topoChangerFvMesh
dynamicMesh
dynamicFvMesh
RBFMotionSolver
tetMotionSolver
fvMotionSolver
mesquiteMotionSolver
solidBodyMotion
finiteArea
POD
coalCombustion
molecule
potential
molecularMeasurements
lagrangian
solidParticle
lagrangianIntermediate
dieselSpray
dsmc
solidModels
coupledLduMatrix
lduSolvers
conversion
foam
sampling
systemCall
checkFunctionObjects
utilityFunctionObjects
forces
fieldFunctionObjects
IOFunctionObjects
foamCalcFunctions
# immersedBoundaryForceFunctionObject
immersedBoundaryDynamicFvMesh
immersedBoundary
immersedBoundaryTurbulence
surfMesh
multiSolver
ODE
incompressibleTransportModels
interfaceProperties
viscoelasticTransportModels
tetFiniteElement
randomProcesses

FVFunctionObjects
immersedBoundaryDynamicMesh
jobControl
loadBalanceFvMesh
pairPatchAgglomeration
conjugateHeatTransfer
)

  detectDepLib(Fx41 "${Fx41_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(Fx41 "${Fx41_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")
  
  set(Fx41_LIBSRC_DIR "${Fx41_DIR}/src")
  set(Fx41_LIB_DIR "${Fx41_DIR}/lib/${Fx41_WM_OPTIONS}")
  
  set(Fx41_INSIGHT_INSTALL_BIN "bin/${Fx41_WM_PROJECT}-${Fx41_WM_PROJECT_VERSION}")
  set(Fx41_INSIGHT_INSTALL_LIB "lib/${Fx41_WM_PROJECT}-${Fx41_WM_PROJECT_VERSION}")
  set(Fx41_INSIGHT_BIN "${CMAKE_BINARY_DIR}/${Fx41_INSIGHT_INSTALL_BIN}")
  set(Fx41_INSIGHT_LIB "${CMAKE_BINARY_DIR}/${Fx41_INSIGHT_INSTALL_LIB}")
  
  addOFConfig(Fx41 fx41 164)

  macro (setup_exe_target_Fx41 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx41_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx41_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx41_LINKEXE}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${Fx41_INSIGHT_BIN})
    set_target_properties(${targetname} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    target_link_libraries(${targetname}
        ${Fx41_LIBRARIES}
        ${ARGN}
        ${Fx41_MESQUITE_LIB_DIR}/libmesquite.so
        ${Fx41_PARMGRIDGEN_LIB_DIR}/libMGridGen.so
        ${Fx41_PARMGRIDGEN_LIB_DIR}/libIMlib.so
     )
     install(TARGETS ${targetname} RUNTIME DESTINATION ${Fx41_INSIGHT_INSTALL_BIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
     
     set_directory_properties(LINK_DIRECTORIES ${temp})
     get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  macro (setup_lib_target_Fx41 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx41_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx41_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx41_LINKLIBSO}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${Fx41_INSIGHT_LIB})
    set_target_properties(${targetname} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    target_link_libraries(${targetname} 
        ${Fx41_LIBRARIES}
        ${ARGN}
        ${Fx41_MESQUITE_LIB_DIR}/libmesquite.so
        ${Fx41_PARMGRIDGEN_LIB_DIR}/libMGridGen.so
        ${Fx41_PARMGRIDGEN_LIB_DIR}/libIMlib.so
    )
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${Fx41_INSIGHT_INSTALL_LIB} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(Fx41_FOUND TRUE)
ENDIF(Fx41_BASHRC)
