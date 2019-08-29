# Try to find OpenFOAM-1.6-ext
# Once done this will define
#
# Fx40_FOUND          - system has foam-extend-4.0 installed
# Fx40_LIBRARIES      - all OpenFOAM libraries
# Fx40_INCLUDE_PATHS  - all OpenFOAM include paths

include(OpenFOAMfuncs)

FIND_FILE(Fx40_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/foam/foam-extend-4.0
  $ENV{HOME}/OpenFOAM/foam-extend-4.0
  /opt/foam/foam-extend-4.0
  /opt/OpenFOAM/foam-extend-4.0
)

message(STATUS "Found foam-extend-4.0 installation: " ${Fx40_BASHRC})

SET(Fx40_FOUND FALSE)

IF(Fx40_BASHRC)

  GET_FILENAME_COMPONENT(Fx40_ETC_DIR ${Fx40_BASHRC} PATH)
  GET_FILENAME_COMPONENT(Fx40_DIR ${Fx40_ETC_DIR} PATH)

  detectEnvVars(Fx40 WM_PROJECT WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(Fx40 LINKLIBSO LINKLIBSO_full)
  detectEnvVar(Fx40 LINKEXE LINKEXE_full)
  detectEnvVar(Fx40 FOAM_MPI MPI)
  detectEnvVar(Fx40 c++FLAGS CXX_FLAGS)

  set(Fx40_CXX_FLAGS "${Fx40_CXX_FLAGS} -DFx40 -DOF16ext -DOF_VERSION=010603 -DOF_FORK_extend")
  set(Fx40_LIBSRC_DIR "${Fx40_DIR}/src")
  set(Fx40_LIB_DIR "${Fx40_DIR}/platforms/${Fx40_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" Fx40_LINKLIBSO ${Fx40_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" Fx40_LINKEXE ${Fx40_LINKEXE_full})
  
  detectIncPaths(Fx40)

  setOFlibvar(Fx40 
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
)

  detectDepLib(Fx40 "${Fx40_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(Fx40 "${Fx40_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")
  
  set(Fx40_LIBSRC_DIR "${Fx40_DIR}/src")
  set(Fx40_LIB_DIR "${Fx40_DIR}/lib/${Fx40_WM_OPTIONS}")
  
  set(Fx40_INSIGHT_INSTALL_BIN "bin/${Fx40_WM_PROJECT}-${Fx40_WM_PROJECT_VERSION}")
  set(Fx40_INSIGHT_INSTALL_LIB "lib/${Fx40_WM_PROJECT}-${Fx40_WM_PROJECT_VERSION}")
  set(Fx40_INSIGHT_BIN "${CMAKE_BINARY_DIR}/${Fx40_INSIGHT_INSTALL_BIN}")
  set(Fx40_INSIGHT_LIB "${CMAKE_BINARY_DIR}/${Fx40_INSIGHT_INSTALL_LIB}")

  addOFConfig(Fx40 fx40 163)

  macro (setup_exe_target_Fx40 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx40_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx40_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx40_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${Fx40_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${Fx40_LIBRARIES}
      ${ARGN}
#      ${Fx40_PARMETIS_LIB_DIR}/libparmetis.a
#      ${Fx40_SCOTCH_LIB_DIR}/libscotch.so
#      ${Fx40_SCOTCH_LIB_DIR}/libscotcherr.so
#      ${Fx40_MESQUITE_LIB_DIR}/libmesquite.so
     )
     install(TARGETS ${targetname} RUNTIME DESTINATION ${Fx40_INSIGHT_INSTALL_BIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
     
     set_directory_properties(LINK_DIRECTORIES ${temp})
     get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  macro (setup_lib_target_Fx40 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    SET(LIB_SEARCHFLAGS "-L${Fx40_LIB_DIR} -L${Fx40_FOAM_MPI_LIBBIN} -L${Fx40_METIS_LIB_DIR} -L${Fx40_PARMETIS_LIB_DIR} -L${Fx40_SCOTCH_LIB_DIR} -L${Fx40_MESQUITE_LIB_DIR}")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx40_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx40_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx40_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${Fx40_INSIGHT_LIB})
    target_link_libraries(${targetname} ${Fx40_LIBRARIES} ${ARGN}) 
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${Fx40_INSIGHT_INSTALL_LIB} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(Fx40_FOUND TRUE)
ENDIF(Fx40_BASHRC)
