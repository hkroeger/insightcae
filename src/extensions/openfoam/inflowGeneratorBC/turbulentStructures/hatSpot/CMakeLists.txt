 
add_sources(inflowGeneratorBC_SOURCES 
  hatSpot.C
)

list (APPEND inflowGeneratorBC_INC_DIRS
 ${PROJECT_SOURCE_DIR}/turbulentStructures/hatSpot
)
set (inflowGeneratorBC_INC_DIRS ${inflowGeneratorBC_INC_DIRS} PARENT_SCOPE)
