# Try to find OCC
# Once done this will define
#
# OCC_FOUND          - system has OCC - OpenCASCADE
# OCC_INCLUDE_DIR    - where the OCC include directory can be found
# OCC_LIBRARY_DIR    - where the OCC library directory can be found
# OCC_LIBRARIES      - Link this to use OCC
# OCC_OCAF_LIBRARIES - Link this to use OCC OCAF framework


  FIND_PATH(DXFLIB_INCLUDE_DIR dl_dxf.h
    /usr/include/dxflib
    /usr/local/include/dxflib
    ENV CPLUS_INCLUDE_PATH
  )

  FIND_LIBRARY(DXFLIB_LIBRARY libdxflib.so
    /usr/lib
    /usr/local/lib
    ENV LD_LIBRARY_PATH
  )


SET(DXFLIB_FOUND FALSE)
IF(DXFLIB_LIBRARY)
  GET_FILENAME_COMPONENT(DXFLIB_LIBRARY_DIR ${DXFLIB_LIBRARY} PATH)
  SET(DXFLIB_FOUND TRUE)
  set(DXFLIB_LIBRARIES
    ${DXFLIB_LIBRARY}
  )
ENDIF(DXFLIB_LIBRARY)

