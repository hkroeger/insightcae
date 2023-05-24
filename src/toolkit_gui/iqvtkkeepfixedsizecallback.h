#ifndef IQVTKKEEPFIXEDSIZECALLBACK_H
#define IQVTKKEEPFIXEDSIZECALLBACK_H


#include "vtkCommand.h"
#include "vtkSmartPointer.h"
#include "vtkTransformFilter.h"

class IQCADModel3DViewer;

class IQVTKKeepFixedSize
: public vtkCommand
{

    const IQCADModel3DViewer* viewer_;
    double initialScale_;
    vtkSmartPointer<vtkTransformFilter> trsf_;

    IQVTKKeepFixedSize();

public:

    static IQVTKKeepFixedSize* New();

    void SetViewer(const IQCADModel3DViewer* viewer);
    void SetInputData(vtkPolyData* inputData);
    void SetInputConnection(vtkAlgorithmOutput* inputPort);

    vtkAlgorithmOutput* GetOutputPort();

    void Execute (vtkObject *caller, unsigned long eventId, void *callData) override;
};

#endif // IQVTKKEEPFIXEDSIZECALLBACK_H
