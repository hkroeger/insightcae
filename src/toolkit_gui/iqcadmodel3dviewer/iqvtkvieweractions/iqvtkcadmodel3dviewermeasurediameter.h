#ifndef IQVTKCADMODEL3DVIEWERMEASUREDIAMETER_H
#define IQVTKCADMODEL3DVIEWERMEASUREDIAMETER_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"

class IQVTKCADModel3DViewer;

class IQVTKCADModel3DViewerMeasureDiameter
: public ViewWidgetAction<IQVTKCADModel3DViewer>
{
    std::shared_ptr<insight::cad::Feature> feat_;

public:
    IQVTKCADModel3DViewerMeasureDiameter(IQVTKCADModel3DViewer &viewWidget);
    ~IQVTKCADModel3DViewerMeasureDiameter();

    void start() override;
};

#endif // IQVTKCADMODEL3DVIEWERMEASUREDIAMETER_H
