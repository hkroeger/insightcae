#ifndef IQVTKVIEWERSTATE_H
#define IQVTKVIEWERSTATE_H

#include <set>


class IQVTKCADModel3DViewer;
class vtkProp;

class IQVTKViewerState
{
protected:
    IQVTKCADModel3DViewer& viewer_;

public:
    IQVTKViewerState(IQVTKCADModel3DViewer& viewer);

    virtual ~IQVTKViewerState();

    IQVTKCADModel3DViewer& viewer() const;

    virtual std::set<vtkProp*> affectedActors() const =0;
};


#endif // IQVTKVIEWERSTATE_H
