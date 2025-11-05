add_sources(insightcad_SOURCES 
  in.cpp
  everything.cpp
  booleanfilters.cpp
  everything.cpp
  extremal.cpp
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


  commonfacearea.h commonfacearea.cpp
)

add_include_dir(INCLUDE_DIRS)
