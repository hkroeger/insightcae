#include "iqvtkcadmodel3dviewerdrawline.h"
#include "base/units.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkconstrainedsketcheditor.h"

#include "cadfeatures/line.h"
#include "datum.h"

#include "vtkMapper.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"



using namespace insight;
using namespace insight::cad;




IQVTKCADModel3DViewerPlanePointBasedAction
::IQVTKCADModel3DViewerPlanePointBasedAction(
        IQVTKConstrainedSketchEditor &editor )
    : IQVTKSelectConstrainedSketchEntity( editor )
{
    setSelectionFilter(
        [](std::weak_ptr<insight::cad::ConstrainedSketchEntity> se)
        {
            return bool(std::dynamic_pointer_cast<insight::cad::SketchPoint>(se.lock()));
        }
    );

    toggleHoveringSelectionPreview(true);

    entitySelected.connect(
        [this](std::weak_ptr<insight::cad::ConstrainedSketchEntity> e)
        {
            insight::dbg()<<"picked existing point"<<std::endl;
            existingPointSelected(
                std::dynamic_pointer_cast<insight::cad::SketchPoint>(e.lock()));
        }
    );
}




bool IQVTKCADModel3DViewerPlanePointBasedAction::onLeftButtonDown(
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    auto ret = IQVTKSelectConstrainedSketchEntity::onLeftButtonDown(nFlags, point);
    if (!ret)
    {
        auto p2=viewer().pointInPlane2D(
            sketch().plane()->plane(),
                viewer().pointInPlane3D(
                    sketch().plane()->plane(), point) );
        newPointCreated(
            std::make_shared<SketchPoint>(
                sketch().plane(), p2(0), p2(1) ) );

        return true;
    }
    return ret;
}





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
            }
        }
    }
}




insight::cad::SketchPointPtr
IQVTKCADModel3DViewerDrawLine::applyWizards(const QPoint screenPoint) const
{
    return applyWizards(viewer().pointInPlane3D(
        sketch().plane()->plane(), screenPoint) );
}




insight::cad::SketchPointPtr
IQVTKCADModel3DViewerDrawLine::applyWizards(const arma::mat& pip3d) const
{
    arma::mat pip=pip3d;
    arma::mat p22=viewer().pointInPlane2D(sketch().plane()->plane(), pip);

    if (p1_)
    {
        arma::mat p21=viewer().pointInPlane2D(sketch().plane()->plane(), p1_->p->value());

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

    return std::make_shared<SketchPoint>(
            sketch().plane(),
            p22(0), p22(1) );
}




IQVTKCADModel3DViewerDrawLine
::IQVTKCADModel3DViewerDrawLine(
        IQVTKConstrainedSketchEditor &editor )
    : IQVTKCADModel3DViewerPlanePointBasedAction(editor),
      previewLine_(nullptr),
      prevLine_(nullptr)
{
    existingPointSelected.connect(
        [this](SketchPointPtr p)
        {
            if (!p1_)
                setP1(p, true);
            else
                setP2(p, true);
        }
    );

    newPointCreated.connect(
        [this](SketchPointPtr p)
        {
            if (!p1_)
                setP1( applyWizards(p->value()), false);
            else
                setP2( applyWizards(p->value()), false);
        }
    );

    newPreviewEntity.connect(
        [this](std::weak_ptr<insight::cad::ConstrainedSketchEntity> pP)
        {
            if (auto p = std::dynamic_pointer_cast<insight::cad::SketchPoint>(pP.lock()))
            {
                updatePreviewLine(p->value());
            }
        }
    );
}




IQVTKCADModel3DViewerDrawLine::~IQVTKCADModel3DViewerDrawLine()
{
    if (previewLine_)
    {
        previewLine_->SetVisibility(false);
        viewer().renderer()->RemoveActor(previewLine_);
    }
}

void IQVTKCADModel3DViewerDrawLine::start()
{}




void IQVTKCADModel3DViewerDrawLine::onMouseMove(
        Qt::MouseButtons buttons,
        const QPoint point,
        Qt::KeyboardModifiers curFlags )
{
    updatePreviewLine(applyWizards(point)->value());

    IQVTKCADModel3DViewerPlanePointBasedAction
        ::onMouseMove(buttons, point, curFlags);
}




void IQVTKCADModel3DViewerDrawLine::setP1(SketchPointPtr p, bool isExistingPoint)
{
    p1_=std::make_shared_aggr<EndPointProperty>(p, isExistingPoint, nullptr);

    if ( !isExistingPoint )
    {
        sketch().geometry().insert(
            std::dynamic_pointer_cast
            <ConstrainedSketchEntity>( p1_->p ) );
    }
    Q_EMIT endPointSelected(p1_.get(), nullptr);

    sketch().invalidate();
    Q_EMIT updateActors();
}




void IQVTKCADModel3DViewerDrawLine::setP2(SketchPointPtr p, bool isExistingPoint)
{
    p2_ = std::make_shared_aggr<EndPointProperty>(p, isExistingPoint, nullptr);

    if (!isExistingPoint)
    {
        sketch().geometry().insert(
            std::dynamic_pointer_cast
            <ConstrainedSketchEntity>( p2_->p ) );
    }
    Q_EMIT endPointSelected(p2_.get(), p1_->p);

    auto line = std::dynamic_pointer_cast<insight::cad::Line>(
        Line::create(p1_->p, p2_->p) );

    sketch().geometry().insert(line);
    sketch().invalidate();

    Q_EMIT lineAdded(line.get(), prevLine_, p2_.get(), p1_.get() );

    prevLine_=line.get();

    Q_EMIT updateActors();

    // continue with next line
    p1_=p2_;
    p2_.reset();
}




bool IQVTKCADModel3DViewerDrawLine::onRightButtonDown(
        Qt::KeyboardModifiers nFlags,
        const QPoint point )
{
    finishAction();
    return true;
}
