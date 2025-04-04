#include "iqvtkcadmodel3dviewerdrawline.h"
#include "base/units.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkconstrainedsketcheditor.h"

#include "datum.h"
#include "cadfeatures/line.h"


#include "vtkMapper.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"
#include <qnamespace.h>



using namespace insight;
using namespace insight::cad;




void IQVTKCADModel3DViewerDrawLine::updatePreviewLine(const arma::mat& pip3d)
{
    // update preview line
    if (p1_ && !p2_)
    {
        if (!previewLine_)
        {
            // create line
            auto l = vtkSmartPointer<vtkLineSource>::New();
            l->SetPoint1(p1_->p->value().memptr());
            l->SetPoint2(pip3d.memptr());

            previewLine_ = vtkSmartPointer<vtkActor>::New();
            previewLine_->SetMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
            previewLine_->GetMapper()->SetInputConnection(l->GetOutputPort());
            previewLine_->GetProperty()->SetColor(1, 0, 0);
            previewLine_->GetProperty()->SetLineWidth(2);
            viewer().renderer()->AddActor(previewLine_);

            viewer().scheduleRedraw();
        }
        else
        {
            // update line
            if (auto*line = vtkLineSource::SafeDownCast(
                    previewLine_->GetMapper()->GetInputAlgorithm()))
            {
                line->SetPoint1(p1_->p->value().memptr());
                line->SetPoint2(pip3d.memptr());
                previewLine_->GetMapper()->Update();

                viewer().scheduleRedraw();
            }
        }
    }
}





IQVTKCADModel3DViewerPlanePointBasedAction::PointProperty
IQVTKCADModel3DViewerDrawLine::applyWizards(
    const arma::mat& pip3d,
    insight::cad::FeaturePtr onFeature ) const
{
    arma::mat pip=pip3d;
    arma::mat p22 =
        viewer().pointInPlane2D(
         sketch().plane()->plane(), pip);

    if (p1_)
    {
        arma::mat p21 =
            viewer().pointInPlane2D(
             sketch().plane()->plane(), p1_->p->value());

        arma::mat l = p22 - p21;
        double angle = atan2(l(1), l(0));

        const double angleGrid=22.5*SI::deg, angleCatch=5.*SI::deg;
        for (double a=-180.*SI::deg; a<180.*SI::deg; a+=angleGrid)
        {
            if (fabs(angle-a)<angleCatch)
            {
                pip=viewer().pointInPlane3D(
                    sketch().plane()->plane(),
                    p21+vec2(cos(a),
                    sin(a))*arma::norm(l,2) );
                break;
            }
        }

        p22=viewer().pointInPlane2D(sketch().plane()->plane(), pip);
    }

    return {
        SketchPoint::create(
            sketch().plane(),
            p22(0), p22(1) ), false, nullptr};
}




IQVTKCADModel3DViewerDrawLine
::IQVTKCADModel3DViewerDrawLine(
        IQVTKConstrainedSketchEditor &editor )
    : IQVTKCADModel3DViewerPlanePointBasedAction(editor),
      previewLine_(nullptr),
      prevLine_(nullptr)
{}




IQVTKCADModel3DViewerDrawLine::~IQVTKCADModel3DViewerDrawLine()
{
    if (previewLine_)
    {
        previewLine_->SetVisibility(false);
        viewer().renderer()->RemoveActor(previewLine_);
    }
}

void IQVTKCADModel3DViewerDrawLine::start()
{
    pointSelected.connect(
        [this](PointProperty pp)
        {
            if (!p1_)
            {
                p1_=std::make_shared_aggr<PointProperty>(pp);

                if ( !pp.isAnExistingPoint )
                {
                    sketch().insertGeometry(
                        std::dynamic_pointer_cast
                        <ConstrainedSketchEntity>( p1_->p ) );
                }
                Q_EMIT endPointSelected(p1_.get(), nullptr);

                sketch().invalidate();
            }
            else
            {
                p2_ = std::make_shared_aggr<PointProperty>(pp);

                if (!pp.isAnExistingPoint)
                {
                    sketch().insertGeometry(
                        std::dynamic_pointer_cast
                        <ConstrainedSketchEntity>( p2_->p ) );
                }
                Q_EMIT endPointSelected(p2_.get(), p1_->p);

                auto line = Line::create(p1_->p, p2_->p);

                sketch().insertGeometry(line);
                sketch().invalidate();

                Q_EMIT lineAdded(line, prevLine_, p2_.get(), p1_.get() );

                prevLine_=line.get();

                // continue with next line
                p1_=p2_;
                p2_.reset();

                if (p1_->isAnExistingPoint) finishAction();
            }
        }
        );

    newPreviewEntity.connect(
        [this](std::weak_ptr<insight::cad::ConstrainedSketchEntity> pP)
        {
            if (auto p =
                std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                    pP.lock() ) )
            {
                updatePreviewLine(p->value());
            }
        }
        );
}




bool IQVTKCADModel3DViewerDrawLine::onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags )
{
    updatePreviewLine(
        IQVTKCADModel3DViewerPlanePointBasedAction::applyWizards(point, nullptr)
            .p->value() );

    return IQVTKCADModel3DViewerPlanePointBasedAction
        ::onMouseMove(point, curFlags);
}





bool IQVTKCADModel3DViewerDrawLine::onMouseClick  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    if (btn==Qt::RightButton)
    {
        finishAction();
        return true;
    }

    return IQVTKCADModel3DViewerPlanePointBasedAction
        ::onMouseClick(btn, nFlags, point);
}
