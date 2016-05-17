# Try to find OCC
# Once done this will define
#
# OCC_FOUND          - system has OCC - OpenCASCADE
# OCC_INCLUDE_DIR    - where the OCC include directory can be found
# OCC_LIBRARY_DIR    - where the OCC library directory can be found
# OCC_LIBRARIES      - Link this to use OCC
# OCC_OCAF_LIBRARIES - Link this to use OCC OCAF framework


IF (WIN32)
  IF (CYGWIN OR MINGW)

    FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
      HINTS /usr/include
      /usr/local/include
      /opt/opencascade/include
      /opt/opencascade/inc
      ENV CPLUS_INCLUDE_PATH
      PATH_SUFFIXES oce opencascade
    )

    FIND_LIBRARY(OCC_LIBRARY TKernel
      HINTS /usr/lib
      /usr/local/lib
      /opt/opencascade/lib
      ENV LD_LIBRARY_PATH
    )

  ELSE (CYGWIN OR MINGW)

    FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SIM\\OCC\\2;Installation Path]/include"
    )

    FIND_LIBRARY(OCC_LIBRARY TKernel
      "[HKEY_LOCAL_MACHINE\\SOFTWARE\\SIM\\OCC\\2;Installation Path]/lib"
    )

  ENDIF (CYGWIN OR MINGW)

ELSE (WIN32)

  FIND_PATH(OCC_INCLUDE_DIR Standard_Version.hxx
    HINTS /usr/include
    /usr/include
    /usr/local/include
    /usr/local/include
    /opt/opencascade/include
    ENV CPLUS_INCLUDE_PATH
    PATH_SUFFIXES oce opencascade
  )

  FIND_LIBRARY(OCC_LIBRARY TKernel
    /usr/lib
    /usr/local/lib
    /opt/opencascade/lib
    ENV LD_LIBRARY_PATH
  )

ENDIF (WIN32)


SET(OCC_FOUND FALSE)
IF(OCC_LIBRARY)
  GET_FILENAME_COMPONENT(OCC_LIBRARY_DIR ${OCC_LIBRARY} PATH)
  SET(OCC_FOUND TRUE)
  set(OCC_LIB_NAMES
    TKFillet
    TKMesh
    TKernel
    TKG2d
    TKG3d
    TKMath
    TKIGES
    TKSTL
    TKShHealing
    TKXSBase
    TKBool
    TKBO
    TKBRep
    TKTopAlgo
    TKGeomAlgo
    TKGeomBase
    TKOffset
    TKPrim
    TKSTEP
    TKSTEPBase
    TKSTEPAttr
    TKHLR
    TKFeat
  )
  set(OCC_OCAF_LIB_NAMES
    TKV3d 
    TKV2d 
    TKOpenGl
    TKService
    TKCAF
    TKXCAF
    TKLCAF
    TKXDESTEP
    TKXDEIGES
    TKMeshVS
    TKAdvTools
  )
  SET(OCC_LIBRARIES "")
   FOREACH(f ${OCC_LIB_NAMES})
    IF (EXISTS "${OCC_LIBRARY_DIR}/lib${f}.so")
      LIST(APPEND OCC_LIBRARIES "${OCC_LIBRARY_DIR}/lib${f}.so")
    endif()
   ENDFOREACH(f)
  SET(OCC_OCAF_LIBRARIES "")
   FOREACH(f ${OCC_OCAF_LIB_NAMES})
    IF (EXISTS "${OCC_LIBRARY_DIR}/lib${f}.so")
      LIST(APPEND OCC_OCAF_LIBRARIES "${OCC_LIBRARY_DIR}/lib${f}.so")
    endif()
   ENDFOREACH(f)
ENDIF(OCC_LIBRARY)

