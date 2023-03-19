#ifndef IQVTKVIEWERSTATE_H
#define IQVTKVIEWERSTATE_H


class IQVTKCADModel3DViewer;

class IQVTKViewerState
{
protected:
    IQVTKCADModel3DViewer& viewer_;

public:
    IQVTKViewerState(IQVTKCADModel3DViewer& viewer);

    virtual ~IQVTKViewerState();

    IQVTKCADModel3DViewer& viewer() const;
};


#endif // IQVTKVIEWERSTATE_H
