# Try to find OCC
# Once done this will define
#
# OCC_FOUND          - system has OCC - OpenCASCADE
# OCC_INCLUDE_DIR    - where the OCC include directory can be found
# OCC_LIBRARY_DIR    - where the OCC library directory can be found
# OCC_LIBRARIES      - Link this to use OCC
# OCC_OCAF_LIBRARIES - Link this to use OCC OCAF framework


FIND_PATH(GEOM_INCLUDE_DIR GEOM_Application.hxx
  /usr/include/GEOM
  /usr/local/include/GEOM
)

FIND_LIBRARY(GEOM_LIBRARY libTKGeom.so
  /usr/lib
  /usr/local/lib
)


SET(GEOM_FOUND FALSE)
IF(GEOM_LIBRARY)
  GET_FILENAME_COMPONENT(GEOM_LIBRARY_DIR ${GEOM_LIBRARY} PATH)
  SET(GEOM_FOUND TRUE)
  set(GEOM_LIBRARIES
TKExchange3DS
TKExchangeBREP
TKExchangeCSFDB
TKExchangeIGES
TKExchangeOBJ
TKExchangeSTEP
TKExchangeSTL
TKExchangeVRML
TKGeom
TKXBO
  )
  set(GEOM_INCLUDE_DIRS
   ${GEOM_INCLUDE_DIR}
   )
ENDIF(GEOM_LIBRARY)

