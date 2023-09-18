
#include "iqvtkcadmodel3dviewer.h"
#include "cadfeature.h"

#include "vtkActor.h"
#include "vtkMapper.h"



IQVTKCADModel3DViewer::ClippingPlanes::ClippingPlanes(
        IQVTKCADModel3DViewer &viewer,
        const arma::mat &p0,
        const arma::mat &n )
    : viewer_(viewer)
{
    auto pl = vtkSmartPointer<vtkPlane>::New();
    pl->SetOrigin(p0.memptr());
    pl->SetNormal(n.memptr());
    insert(pl);

    for (const auto& pp: viewer_.displayedData_)
    {
        for (const auto& act: pp.second.actors_)
        {
            if (auto* actor = vtkActor::SafeDownCast(act))
            {
                for (const auto& pl: *this)
                {
                    actor->GetMapper()->AddClippingPlane(pl);
                }
            }
        }
    }
    viewer_.scheduleRedraw();
}

IQVTKCADModel3DViewer::ClippingPlanes::~ClippingPlanes()
{
    for (const auto& pp: viewer_.displayedData_)
    {
        for (const auto& act: pp.second.actors_)
        {
            if (auto* actor = vtkActor::SafeDownCast(act))
            {
                for (const auto& pl: *this)
                {
                    actor->GetMapper()->RemoveClippingPlane(pl);
                }
            }
        }
    }
    viewer_.scheduleRedraw();
}
