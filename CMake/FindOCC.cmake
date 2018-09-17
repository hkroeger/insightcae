# Try to find OCC
# Once done this will define
#
# OCC_FOUND          - system has OCC - OpenCASCADE
# OCC_INCLUDE_DIR    - where the OCC include directory can be found
# OCC_LIBRARY_DIR    - where the OCC library directory can be found
# OCC_LIBRARIES      - Link this to use OCC
# OCC_OCAF_LIBRARIES - Link this to use OCC OCAF framework

if (NOT INSIGHT_FORCE_OCC)
 find_package(OCE)
else()
 set(OCE_FOUND FALSE)
endif()

if (OCE_FOUND)
 set(OCC_LIBRARIES ${OCE_LIBRARIES})
 set(OCC_INCLUDE_DIRS ${OCE_INCLUDE_DIR} ${OCE_INCLUDE_DIRS})
 set(OCC_INCLUDE_DIR ${OCC_INCLUDE_DIRS})
else()
message(STATUS "OCE not found, looking for OpenCASCADE")
 find_package(OpenCASCADE)
 set(OCC_FOUND ${OpenCASCADE_FOUND})
 set(OCC_LIBRARIES ${OpenCASCADE_FoundationClasses_LIBRARIES} ${OpenCASCADE_ModelingData_LIBRARIES} ${OpenCASCADE_ModelingAlgorithms_LIBRARIES} ${OpenCASCADE_Visualization_LIBRARIES} ${OpenCASCADE_DataExchange_LIBRARIES} ${OpenCASCADE_ApplicationFramework_LIBRARIES})
 set(OCC_INCLUDE_DIRS ${OpenCASCADE_INCLUDE_DIR})
 set(OCC_INCLUDE_DIR ${OpenCASCADE_INCLUDE_DIR})
 
endif()
