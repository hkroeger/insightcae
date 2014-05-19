
# include all addon subdirectories that exist
FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

FOREACH(child ${children})
  IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
      ADD_SUBDIRECTORY(${child})
  ENDIF()
ENDFOREACH()
