# Try to find OCC
# Once done this will define
#
# OCC_FOUND          - system has OCC - OpenCASCADE
# OCC_INCLUDE_DIR    - where the OCC include directory can be found
# OCC_LIBRARY_DIR    - where the OCC library directory can be found
# OCC_LIBRARIES      - Link this to use OCC
# OCC_OCAF_LIBRARIES - Link this to use OCC OCAF framework


FIND_PATH(DLIB_INCLUDE_DIR dlib/optimization.h
  HINTS /usr/include /usr/local/include
  ENV CPLUS_INCLUDE_PATH
  #PATH_SUFFIXES dlib
)

FIND_LIBRARY(DLIB_LIBRARY libdlib.so
  HINTS /usr/lib /usr/lib64 /usr/lib32 /usr/local/lib
  ENV LD_LIBRARY_PATH
)


SET(DLIB_FOUND FALSE)
IF(DLIB_LIBRARY)
  GET_FILENAME_COMPONENT(DLIB_INCLUDE_DIR ${DLIB_INCLUDE_DIR} PATH)
  message(STATUS "DLIB=${DLIB_INCLUDE_DIR}")
  GET_FILENAME_COMPONENT(DLIB_LIBRARY_DIR ${DLIB_LIBRARY} PATH)
  SET(DLIB_FOUND TRUE)
  set(DLIB_LIBRARIES
    ${DLIB_LIBRARY}
  )
ENDIF(DLIB_LIBRARY)

