add_sources(insightcad_SOURCES 
  in.cpp
  everything.cpp
  booleanfilters.cpp
  everything.cpp
  minimal.cpp
  maximal.cpp
  
  connected.cpp
  coincident.cpp
  identical.cpp identical.h
  same.cpp same.h
  ispartofsolid.cpp
  ispartofface.cpp

  boundaryedge.cpp
  edgetopology.cpp
  #secant.cpp
  coincidentprojectededge.cpp
  edgeconnectingvertices.cpp edgeconnectingvertices.h
  
  facetopology.cpp
  faceadjacenttoedges.cpp
  faceadjacenttofaces.cpp
  boundaryofface.cpp
  cylfaceorientation.cpp
)

add_include_dir(INCLUDE_DIRS)
