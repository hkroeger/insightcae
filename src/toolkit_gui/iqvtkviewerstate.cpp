#include "iqvtkviewerstate.h"

IQVTKViewerState::IQVTKViewerState(IQVTKCADModel3DViewer& viewer)
    : viewer_(viewer)
{}

IQVTKViewerState::~IQVTKViewerState()
{}

IQVTKCADModel3DViewer& IQVTKViewerState::viewer() const
{
    return viewer_;
}
