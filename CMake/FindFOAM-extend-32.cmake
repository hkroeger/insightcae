# Try to find foam-extend-3.2
# Once done this will define
#
# Fx32_FOUND          - system has foam-extend-3.2 installed
# Fx32_LIBRARIES      - all OpenFOAM libraries
# Fx32_INCLUDE_PATHS  - all OpenFOAM include paths

include(OpenFOAMfuncs)

FIND_FILE(Fx32_BASHRC NAMES etc/bashrc
  HINTS
  $ENV{HOME}/foam/foam-extend-3.2
  $ENV{HOME}/OpenFOAM/foam-extend-3.2
  /opt/foam/foam-extend-3.2
  /opt/OpenFOAM/foam-extend-3.2
)

message(STATUS "Found foam-extend-3.2 installation: " ${Fx32_BASHRC})

SET(Fx32_FOUND FALSE)

IF(Fx32_BASHRC)

  GET_FILENAME_COMPONENT(Fx32_ETC_DIR ${Fx32_BASHRC} PATH)
  GET_FILENAME_COMPONENT(Fx32_DIR ${Fx32_ETC_DIR} PATH)

  detectEnvVars(Fx32 WM_PROJECT WM_PROJECT_VERSION WM_OPTIONS FOAM_EXT_LIBBIN SCOTCH_ROOT FOAM_APPBIN FOAM_LIBBIN)
  detectEnvVar(Fx32 LINKLIBSO LINKLIBSO_full)
  detectEnvVar(Fx32 LINKEXE LINKEXE_full)
  detectEnvVar(Fx32 FOAM_MPI MPI)
  detectEnvVar(Fx32 c++FLAGS CXX_FLAGS)

  set(Fx32_CXX_FLAGS "${Fx32_CXX_FLAGS} -DFx32 -DOF16ext")
  set(Fx32_LIBSRC_DIR "${Fx32_DIR}/src")
  set(Fx32_LIB_DIR "${Fx32_DIR}/platforms/${Fx32_WM_OPTIONS}/lib")
  
  string(REGEX REPLACE "^[^ ]+" "" Fx32_LINKLIBSO ${Fx32_LINKLIBSO_full})
  string(REGEX REPLACE "^[^ ]+" "" Fx32_LINKEXE ${Fx32_LINKEXE_full})
  
  detectIncPaths(Fx32)

  setOFlibvar(Fx32 
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

  detectDepLib(Fx32 "${Fx32_FOAM_LIBBIN}/libfiniteVolume.so" "Pstream")
  detectDepLib(Fx32 "${Fx32_FOAM_LIBBIN}/libscotchDecomp.so" "scotch")
  
  set(Fx32_LIBSRC_DIR "${Fx32_DIR}/src")
  set(Fx32_LIB_DIR "${Fx32_DIR}/lib/${Fx32_WM_OPTIONS}")
  
  set(Fx32_INSIGHT_INSTALL_BIN "bin/${Fx32_WM_PROJECT}-${Fx32_WM_PROJECT_VERSION}")
  set(Fx32_INSIGHT_INSTALL_LIB "lib/${Fx32_WM_PROJECT}-${Fx32_WM_PROJECT_VERSION}")
  set(Fx32_INSIGHT_BIN "${CMAKE_BINARY_DIR}/${Fx32_INSIGHT_INSTALL_BIN}")
  set(Fx32_INSIGHT_LIB "${CMAKE_BINARY_DIR}/${Fx32_INSIGHT_INSTALL_LIB}")
  
  addOFConfig(Fx32 fx32 162)

  macro (setup_exe_target_Fx32 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    add_executable(${targetname} ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx32_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx32_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx32_LINKEXE} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${Fx32_INSIGHT_BIN})
    target_link_libraries(${targetname} 
      ${Fx32_LIBRARIES}
      ${ARGN}
#      ${Fx32_PARMETIS_LIB_DIR}/libparmetis.a
#      ${Fx32_SCOTCH_LIB_DIR}/libscotch.so
#      ${Fx32_SCOTCH_LIB_DIR}/libscotcherr.so
#      ${Fx32_MESQUITE_LIB_DIR}/libmesquite.so
     )
     install(TARGETS ${targetname} RUNTIME DESTINATION ${Fx32_INSIGHT_INSTALL_BIN} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
     
     set_directory_properties(LINK_DIRECTORIES ${temp})
     get_directory_property(temp LINK_DIRECTORIES)
  endmacro()
  
  macro (setup_lib_target_Fx32 targetname sources exename includes)
    get_directory_property(temp LINK_DIRECTORIES)
    
    SET(LIB_SEARCHFLAGS "-L${Fx32_LIB_DIR} -L${Fx32_FOAM_MPI_LIBBIN} -L${Fx32_METIS_LIB_DIR} -L${Fx32_PARMETIS_LIB_DIR} -L${Fx32_SCOTCH_LIB_DIR} -L${Fx32_MESQUITE_LIB_DIR}")
    
    add_library(${targetname} SHARED ${sources})
    
    set(allincludes ${includes})
    LIST(APPEND allincludes "${Fx32_INCLUDE_PATHS}")
    set_target_properties(${targetname} PROPERTIES INCLUDE_DIRECTORIES "${allincludes}")
    set_target_properties(${targetname} PROPERTIES COMPILE_FLAGS ${Fx32_CXX_FLAGS})
    set_target_properties(${targetname} PROPERTIES LINK_FLAGS "${Fx32_LINKLIBSO} ${LIB_SEARCHFLAGS}")
    set_target_properties(${targetname} PROPERTIES OUTPUT_NAME ${exename})
    set_target_properties(${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${Fx32_INSIGHT_LIB})
    target_link_libraries(${targetname} ${Fx32_LIBRARIES} ${ARGN}) 
    target_include_directories(${targetname}
      PUBLIC ${CMAKE_CURRENT_BINARY_DIR} 
      PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    )
    install(TARGETS ${targetname} LIBRARY DESTINATION ${Fx32_INSIGHT_INSTALL_LIB} COMPONENT ${INSIGHT_INSTALL_COMPONENT})
    
    set_directory_properties(LINK_DIRECTORIES ${temp})
  endmacro()
  
  SET(Fx32_FOUND TRUE)
ENDIF(Fx32_BASHRC)
