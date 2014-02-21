 
add_sources(LESFunctionObjects_SOURCES 
  twoPointCorrelation.C
)

list (APPEND LESFunctionObjects_INC_DIRS
 ${LESFunctionObjects_SOURCE_DIR}/twoPointCorrelation
)
set (LESFunctionObjects_INC_DIRS ${LESFunctionObjects_INC_DIRS} PARENT_SCOPE)
