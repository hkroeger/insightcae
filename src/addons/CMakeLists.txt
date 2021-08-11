
# include all addon subdirectories that exist
FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

FOREACH(child ${children})
  IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${child})
    OPTION(INSIGHT_MODULE_${child} "Include module ${child}" ON)
    IF (INSIGHT_MODULE_${child})
      ADD_SUBDIRECTORY(${child})
    ENDIF()
  ENDIF()
ENDFOREACH()

SET(git-rev ${git-rev} PARENT_SCOPE)
