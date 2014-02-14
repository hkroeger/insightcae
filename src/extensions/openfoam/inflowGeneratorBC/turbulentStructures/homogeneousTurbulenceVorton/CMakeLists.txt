 
add_sources(inflowGeneratorBC_SOURCES 
  homogeneousTurbulenceVorton.C
)

list (APPEND inflowGeneratorBC_INC_DIRS
 ${inflowGeneratorBC_SOURCE_DIR}/turbulentStructures/homogeneousTurbulenceVorton
)
set (inflowGeneratorBC_INC_DIRS ${inflowGeneratorBC_INC_DIRS} PARENT_SCOPE)
