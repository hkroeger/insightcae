 
add_sources(inflowGeneratorBC_SOURCES 
  hatSpot.C
)

list (APPEND inflowGeneratorBC_INC_DIRS
 ${inflowGeneratorBC_SOURCE_DIR}/turbulentStructures/hatSpot
)
set (inflowGeneratorBC_INC_DIRS ${inflowGeneratorBC_INC_DIRS} PARENT_SCOPE)
