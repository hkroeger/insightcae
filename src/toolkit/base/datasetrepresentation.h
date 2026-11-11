#ifndef INSIGHT_DATASETREPRESENTATION_H
#define INSIGHT_DATASETREPRESENTATION_H

namespace insight
{

/**
 * Representation style of a VTK dataset.
 *
 * Kept in this separate, dependency-free header so that it can be used
 * in widely included headers (e.g. cadtypes.h) without pulling in
 * vtkrendering.h and the VTK rendering headers.
 * The values equal VTK_POINTS, VTK_WIREFRAME and VTK_SURFACE
 * (checked in vtkrendering.cpp).
 */
enum DatasetRepresentation
{
  Points = 0,
  Wireframe = 1,
  Surface = 2
};

}

#endif // INSIGHT_DATASETREPRESENTATION_H
