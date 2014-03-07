 
add_sources(inflowGeneratorBC_SOURCES 
  turbulentStructure.C
)

list (APPEND inflowGeneratorBC_INC_DIRS
  ${PROJECT_SOURCE_DIR}/turbulentStructures/turbulentStructure
)
set (inflowGeneratorBC_INC_DIRS ${inflowGeneratorBC_INC_DIRS} PARENT_SCOPE)
