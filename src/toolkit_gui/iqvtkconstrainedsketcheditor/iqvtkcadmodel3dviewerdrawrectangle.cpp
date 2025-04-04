#include "iqvtkcadmodel3dviewerdrawrectangle.h"

#include "datum.h"

#include "vtkPolyLineSource.h"
#include <qnamespace.h>

using namespace insight;
using namespace insight::cad;

std::pair<arma::mat, arma::mat>
IQVTKCADModel3DViewerDrawRectangle::calcP2P4(const arma::mat& p2_3d) const
{
    auto plane = std::dynamic_pointer_cast<DatumPlaneData>(sketch().plane());
    arma::mat ex = plane->ex();
    arma::mat ey = plane->ey();

    arma::mat p0=p1_->p->value();
    arma::mat diag=p2_3d-p0;
    return {
      arma::dot(diag,ex)*ex+p0,
      arma::dot(diag,ey)*ey+p0
    };
}

void IQVTKCADModel3DViewerDrawRectangle::updatePreviewRect(const arma::mat& point3d)
{
    // update preview line
    if (p1_ && !p2_)
    {
        auto plane = std::dynamic_pointer_cast<DatumPlaneData>(sketch().plane());
        arma::mat ex = plane->ex();
        arma::mat ey = plane->ey();

        std::array<arma::mat, 4> p;
        p[0]=p1_->p->value();
        p[2]=point3d;
        auto p1p3=calcP2P4(point3d);
        p[1]=p1p3.first;
        p[3]=p1p3.second;

        previewUpdated(p[0], p[2]);

        if (!previewLine_)
        {

            auto L = vtkSmartPointer<vtkPolyLineSource>::New();
            // Create a vtkPoints object and store the points in it
            L->SetNumberOfPoints(p.size());
            L->ClosedOn();
            for (auto pd: boost::adaptors::index(p))
            {
                L->SetPoint(
                    pd.index(),
                    pd.value()[0],
                    pd.value()[1],
                    pd.value()[2] );
            }

            previewLine_ = vtkSmartPointer<vtkActor>::New();
            previewLine_->SetMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
            previewLine_->GetMapper()->SetInputConnection(L->GetOutputPort());
            previewLine_->GetProperty()->SetColor(1, 0, 0);
            previewLine_->GetProperty()->SetLineWidth(2);
            viewer().renderer()->AddActor(previewLine_);

            viewer().scheduleRedraw();
        }
        else
        {
            // update line
            if (vtkSmartPointer<vtkPolyLineSource> L = vtkPolyLineSource::SafeDownCast(
                    previewLine_->GetMapper()->GetInputAlgorithm() ))
            {
                for (auto pd: boost::adaptors::index(p))
                {
                    L->SetPoint(
                        pd.index(),
                        pd.value()[0],
                        pd.value()[1],
                        pd.value()[2] );
                }

                // previewLine_->GetMapper()->Update(); // does not work for polylinesource

                viewer().renderer()->RemoveActor(previewLine_); // workaround
                viewer().renderer()->AddActor(previewLine_);

                viewer().scheduleRedraw();
            }
        }
    }
}



IQVTKCADModel3DViewerDrawRectangle::IQVTKCADModel3DViewerDrawRectangle(
    IQVTKConstrainedSketchEditor &editor,
    bool allowExistingPoints,
    bool addToSketch )
  : IQVTKCADModel3DViewerPlanePointBasedAction(editor, allowExistingPoints),
    previewLine_(nullptr),
    prevLine_(nullptr),
    addToSketch_(addToSketch)
{
}


IQVTKCADModel3DViewerDrawRectangle::~IQVTKCADModel3DViewerDrawRectangle()
{
    if (previewLine_)
    {
        previewLine_->SetVisibility(false);
        viewer().renderer()->RemoveActor(previewLine_);
    }
}

void IQVTKCADModel3DViewerDrawRectangle::start()
{
    pointSelected.connect(
        [this](PointProperty pp)
        {
            if (!p1_)
            {
                p1_=std::make_shared_aggr<PointProperty>(pp);

                Q_EMIT updateActors();
            }
            else
            {
                p2_ = std::make_shared_aggr<PointProperty>(pp);

                if (addToSketch_)
                {
                    if ( !p1_->isAnExistingPoint )
                    {
                        sketch().insertGeometry( p1_->p );
                    }

                    if (!p2_->isAnExistingPoint)
                    {
                        sketch().insertGeometry( p2_->p );
                    }

                    auto p2p4 = calcP2P4(p2_->p->value());

                    auto sp2 = SketchPoint::create(
                        sketch().plane(),
                        viewer().pointInPlane2D(
                            sketch().plane()->plane(),
                            p2p4.first ) );
                    sketch().insertGeometry( sp2 );

                    auto sp4 = SketchPoint::create(
                        sketch().plane(),
                        viewer().pointInPlane2D(
                            sketch().plane()->plane(),
                            p2p4.second ) );
                    sketch().insertGeometry( sp4 );


                    auto l1 = Line::create( p1_->p, sp2 );
                    auto l2 = Line::create( sp2, p2_->p );
                    auto l3 = Line::create( p2_->p, sp4 );
                    auto l4 = Line::create( sp4, p1_->p );

                    sketch().insertGeometry(l1);
                    sketch().insertGeometry(l2);
                    sketch().insertGeometry(l3);
                    sketch().insertGeometry(l4);
                    sketch().invalidate();

                    Q_EMIT rectangleAdded({l1, l2, l3, l4}, p2_.get(), p1_.get() );
                    Q_EMIT updateActors();
                }


                finishAction();
            }
        }
        );

    newPreviewEntity.connect(
        [this](std::weak_ptr<insight::cad::ConstrainedSketchEntity> pP)
        {
            if (auto p = std::dynamic_pointer_cast<insight::cad::SketchPoint>(pP.lock()))
            {
                updatePreviewRect(p->value());
            }
        }
        );
}




bool IQVTKCADModel3DViewerDrawRectangle::onMouseMove(
    const QPoint point,
    Qt::KeyboardModifiers curFlags )
{
    updatePreviewRect(
        IQVTKCADModel3DViewerPlanePointBasedAction::applyWizards(point, nullptr)
            .p->value() );

    return IQVTKCADModel3DViewerPlanePointBasedAction
        ::onMouseMove(point, curFlags);
}




bool IQVTKCADModel3DViewerDrawRectangle::onMouseClick  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    if (btn==Qt::RightButton)
    {
        finishAction();
        return true;
    }
    else
        return IQVTKCADModel3DViewerPlanePointBasedAction
            ::onMouseClick( btn, nFlags, point );
}
