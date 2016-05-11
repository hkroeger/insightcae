add_sources(insightcad_SOURCES 
  in.cpp
  everything.cpp
  booleanfilters.cpp
  everything.cpp
  minimal.cpp
  maximal.cpp
  
  coincident.cpp
  identical.cpp
  ispartofsolid.cpp

  boundaryedge.cpp
  edgetopology.cpp
  #secant.cpp
  coincidentprojectededge.cpp
  
  facetopology.cpp
  faceadjacenttoedges.cpp
  faceadjacenttofaces.cpp
  boundaryofface.cpp
  cylfaceorientation.cpp
)

add_include_dir(INCLUDE_DIRS)