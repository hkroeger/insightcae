# Try to find OpenFOAM-2.2.x
# Once done this will define
#
# OF22x_FOUND          - system has OCC - OpenCASCADE
# OF22x_INCLUDE_DIR    - where the OCC include directory can be found
# OF22x_LIBRARY_DIR    - where the OCC library directory can be found
# OF22x_LIBRARIES      - Link this to use OCC



FIND_PATH(OF22x_LIBSRC_DIR NAMES OpenFOAM/lnInclude/polyMesh.H
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2.x/src
  /opt/OpenFOAM/OpenFOAM-2.2.x/src
)

message(STATUS ${OF22x_LIBSRC_DIR})

FIND_LIBRARY(OF22x_LIBRARY NAMES OpenFOAM
  HINTS
  $ENV{HOME}/OpenFOAM/OpenFOAM-2.2.x/platforms/*/lib
  /opt/OpenFOAM/OpenFOAM-2.2.x/platforms/*/lib
)

message(STATUS ${OF22x_LIBRARY})

SET(OF22x_FOUND FALSE)

IF(OF22x_LIBRARY)
  GET_FILENAME_COMPONENT(OF22x_LIB_DIR ${OF22x_LIBRARY} PATH)
  message(STATUS ${OF22x_LIB_DIR})
  SET(OF22x_FOUND TRUE)
  SET(OF22x_CXX_FLAGS "-Dlinux64 -DWM_DP -O3 -DNoRepository -ftemplate-depth-100 -fPIC -fpermissive -DOF22x")
  message(STATUS ${OF22x_CXX_FLAGS})
  SET(OF22x_SHARED_LINKER_FLAGS "-shared -Xlinker --add-needed -Wl,--no-as-needed")
  SET(OF22x_LINK_FLAGS "-Xlinker --add-needed -Wl,--no-as-needed")
  #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/OpenFOAM-2.2.x)
  #set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/OpenFOAM-2.2.x)
ENDIF(OF22x_LIBRARY)

