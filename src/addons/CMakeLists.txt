# Additional directories outside the source tree which contain addons,
# given as a semicolon-separated list, e.g.
#   cmake "-DINSIGHT_ADDON_DIRECTORIES=/path/to/addonsA;/path/to/addonsB" ...
# Each listed directory is treated like this one: every subdirectory
# containing a CMakeLists.txt is added as an addon.
# Relative paths are interpreted relative to the build directory.
SET(INSIGHT_ADDON_DIRECTORIES "" CACHE STRING
    "Semicolon-separated list of additional directories, each containing addon subdirectories")

# add all addon subdirectories of addonParentDir
# (macro, not function, so that variables set by addons stay in this scope)
MACRO(INSIGHT_ADD_ADDONS_FROM addonParentDir)
  FILE(GLOB _addon_children RELATIVE ${addonParentDir} ${addonParentDir}/*)
  LIST(SORT _addon_children)

  FOREACH(_addon_child ${_addon_children})
    SET(_addon_dir ${addonParentDir}/${_addon_child})
    IF(IS_DIRECTORY ${_addon_dir} AND EXISTS ${_addon_dir}/CMakeLists.txt)

      IF(DEFINED _addon_location_${_addon_child})
        MESSAGE(FATAL_ERROR
          "Addon \"${_addon_child}\" found in ${_addon_dir} has already been added from ${_addon_location_${_addon_child}}")
      ENDIF()
      SET(_addon_location_${_addon_child} ${_addon_dir})

      OPTION(INSIGHT_MODULE_${_addon_child} "Include module ${_addon_child}" ON)
      IF (INSIGHT_MODULE_${_addon_child})
        IF("${addonParentDir}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
          ADD_SUBDIRECTORY(${_addon_dir})
        ELSE()
          # out-of-tree source dirs require an explicit binary dir
          MESSAGE(STATUS "Adding external addon ${_addon_child} from ${_addon_dir}")
          ADD_SUBDIRECTORY(${_addon_dir} ${CMAKE_CURRENT_BINARY_DIR}/${_addon_child})
        ENDIF()
      ENDIF()

    ENDIF()
  ENDFOREACH()
ENDMACRO()


# include all addon subdirectories that exist
INSIGHT_ADD_ADDONS_FROM(${CMAKE_CURRENT_SOURCE_DIR})

# include addons from additional directories
FOREACH(_addon_parent ${INSIGHT_ADDON_DIRECTORIES})
  GET_FILENAME_COMPONENT(_addon_parent ${_addon_parent} ABSOLUTE BASE_DIR ${CMAKE_BINARY_DIR})
  IF(NOT IS_DIRECTORY ${_addon_parent})
    MESSAGE(FATAL_ERROR "Addon directory ${_addon_parent} (from INSIGHT_ADDON_DIRECTORIES) does not exist")
  ENDIF()
  INSIGHT_ADD_ADDONS_FROM(${_addon_parent})
ENDFOREACH()
