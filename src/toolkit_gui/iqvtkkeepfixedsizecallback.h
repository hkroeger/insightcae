#ifndef IQVTKKEEPFIXEDSIZECALLBACK_H
#define IQVTKKEEPFIXEDSIZECALLBACK_H

#include <armadillo>

#include "vtkCommand.h"
#include "vtkSmartPointer.h"
#include "vtkTransformFilter.h"

class IQVTKCADModel3DViewer;

class IQVTKKeepFixedSize
: public vtkCommand
{

    arma::mat pref_;
    const IQVTKCADModel3DViewer* viewer_;
    double initialScale_;
    vtkSmartPointer<vtkTransformFilter> trsf_;

    IQVTKKeepFixedSize();

public:

    static IQVTKKeepFixedSize* New();

    void SetViewer(const IQVTKCADModel3DViewer* viewer);
    void SetInputData(vtkPolyData* inputData);
    void SetInputConnection(vtkAlgorithmOutput* inputPort);
    void SetPRef(const arma::mat& pref);

    vtkAlgorithmOutput* GetOutputPort();

    void Execute (vtkObject *caller, unsigned long eventId, void *callData) override;
};

#endif // IQVTKKEEPFIXEDSIZECALLBACK_H
