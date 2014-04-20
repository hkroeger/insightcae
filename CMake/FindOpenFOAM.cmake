# Try to find OpenFOAM
# Once done this will define
#
# OF_FOUND          - system has OCC - OpenCASCADE
# OF_INCLUDE_DIR    - where the OCC include directory can be found
# OF_LIBRARY_DIR    - where the OCC library directory can be found
# OF_LIBRARIES      - Link this to use OCC



FIND_PATH(OF_LIBSRC_DIR NAMES OpenFOAM/lnInclude/polyMesh.H
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-1.6-ext/src
  /opt/OpenFOAM/OpenFOAM-1.6-ext/src
)

message(STATUS ${OF_LIBSRC_DIR})

FIND_LIBRARY(OF_LIBRARY NAMES OpenFOAM
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-1.6-ext/lib/*
  /opt/OpenFOAM/OpenFOAM-1.6-ext/lib/*
)

message(STATUS ${OF_LIBRARY})

SET(OF_FOUND FALSE)

IF(OF_LIBRARY)
  GET_FILENAME_COMPONENT(OF_LIB_DIR ${OF_LIBRARY} PATH)
  message(STATUS ${OF_LIB_DIR})
  SET(OF_FOUND TRUE)
  SET(OF_CXX_FLAGS "-Dlinux64 -DWM_DP -O3 -DNoRepository -ftemplate-depth-40 -fPIC -fpermissive -DOF16ext")
  message(STATUS ${OF_CXX_FLAGS})
  SET(OF_SHARED_LINKER_FLAGS "-shared -Xlinker --add-needed -Wl,--no-as-needed")
  SET(OF_LINK_FLAGS "-Xlinker --add-needed -Wl,--no-as-needed")  
  #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/OpenFOAM-1.6-ext)
  #set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/OpenFOAM-1.6-ext)
ENDIF(OF_LIBRARY)

