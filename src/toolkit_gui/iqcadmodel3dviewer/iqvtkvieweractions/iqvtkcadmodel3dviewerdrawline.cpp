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






SketchPointPtr
IQVTKCADModel3DViewerPlanePointBasedAction
::existingSketchPointAt( const QPoint& cp, insight::cad::ConstrainedSketchEntityPtr& sg ) const
{
    if (auto act =
        viewer().findActorUnderCursorAt(cp))
    {
        if (auto se =
            std::dynamic_pointer_cast<IQVTKConstrainedSketchEditor>(
                viewer().currentAction_))
        {
            if ( (sg =
                 se->findSketchElementOfActor(act)) )
            {
                if (auto sp =
                    std::dynamic_pointer_cast
                    <SketchPoint>(sg))
                {
                    insight::dbg()<<"picked existing point"<<std::endl;
                    return sp;
                }
            }
        }
    }

    return nullptr;
}




IQVTKCADModel3DViewerPlanePointBasedAction
::IQVTKCADModel3DViewerPlanePointBasedAction(
        IQVTKCADModel3DViewer &viewWidget,
        insight::cad::ConstrainedSketchPtr sketch )
    : ViewWidgetAction<IQVTKCADModel3DViewer>(
          viewWidget),
      sketch_(sketch)
{}




IQVTKCADModel3DViewerDrawLine::CandidatePoint
IQVTKCADModel3DViewerDrawLine::updatePCand(const QPoint& point) const
{
    CandidatePoint pc{nullptr, false, nullptr};

    insight::cad::ConstrainedSketchEntityPtr selse;
    pc.sketchPoint = existingSketchPointAt(point, selse);

    if (!pc.sketchPoint && selse)
    {
        pc.onFeature = std::dynamic_pointer_cast<insight::cad::Feature>(selse);
    }

    if (!pc.sketchPoint)
    {
        arma::mat pip=viewer().pointInPlane3D(sketch_->plane()->plane(), point);

        if (p1_ && !p2_)
        {
            if (!pc.sketchPoint) // not over existing point
            {

                arma::mat p21=viewer().pointInPlane2D(sketch_->plane()->plane(), p1_->value());
                arma::mat p22=viewer().pointInPlane2D(sketch_->plane()->plane(), pip);

                arma::mat l = p22 - p21;
                double angle = atan2(l(1), l(0));

                const double angleGrid=22.5*SI::deg, angleCatch=5.*SI::deg;
                for (double a=-180.*SI::deg; a<180.*SI::deg; a+=angleGrid)
                {
                    if (fabs(angle-a)<angleCatch)
                    {
                        pip=viewer().pointInPlane3D(
                            sketch_->plane()->plane(),
                            p21+vec2(cos(a),
                            sin(a))*arma::norm(l,2) );
                        break;
                    }
                }

            }
        }

        auto p2=viewer().pointInPlane2D(sketch_->plane()->plane(), pip);
        pc.sketchPoint = std::make_shared<SketchPoint>(
            sketch_->plane(),
            p2(0), p2(1) );
    }
    else
    {
        pc.isAnExistingPoint=true;
    }

    return pc;
}



IQVTKCADModel3DViewerDrawLine
::IQVTKCADModel3DViewerDrawLine(
        IQVTKCADModel3DViewer &viewWidget,
        insight::cad::ConstrainedSketchPtr sketch )
    : IQVTKCADModel3DViewerPlanePointBasedAction(
          viewWidget, sketch),
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




void IQVTKCADModel3DViewerDrawLine::onMouseMove(
        Qt::MouseButtons buttons,
        const QPoint point,
        Qt::KeyboardModifiers curFlags )
{
    // find candidate point for next click
    pcand_.reset(new CandidatePoint(updatePCand(point)));

    // update preview line
    if (p1_ && !p2_)
    {
        if (!previewLine_)
        {
            // create line
            auto l = vtkSmartPointer<vtkLineSource>::New();
            l->SetPoint1(p1_->value().memptr());
            l->SetPoint2(pcand_->sketchPoint->value().memptr());

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
                line->SetPoint1(p1_->value().memptr());
                line->SetPoint2(pcand_->sketchPoint->value().memptr());
                previewLine_->GetMapper()->Update();
            }
        }
    }
}





bool IQVTKCADModel3DViewerDrawLine::onLeftButtonDown(
        Qt::KeyboardModifiers nFlags,
        const QPoint point )
{

    // should have been set during mouse move. if not, do now
    if (!pcand_)
        pcand_.reset(new CandidatePoint(updatePCand(point)));

    if (!p1_)
    {
        p1_=pcand_->sketchPoint;

        if ( !pcand_->isAnExistingPoint )
        {
            sketch_->geometry().insert(
                 std::dynamic_pointer_cast
                    <ConstrainedSketchEntity>( p1_ ) );
        }
        Q_EMIT endPointSelected(pcand_.get(), nullptr);

        sketch_->invalidate();
        Q_EMIT updateActors();
        p1cand_=std::move(pcand_);
        return true;
    }
    else if (!p2_)
    {
        p2_ = pcand_->sketchPoint;

        if (!pcand_->isAnExistingPoint)
        {
            sketch_->geometry().insert(
                 std::dynamic_pointer_cast
                    <ConstrainedSketchEntity>( p2_ ) );
        }
        Q_EMIT endPointSelected(pcand_.get(), p1_);

        auto line = std::dynamic_pointer_cast<insight::cad::Line>(
                    Line::create(p1_, p2_) );
        sketch_->geometry().insert(line);
        sketch_->invalidate();

        Q_EMIT lineAdded(line.get(), prevLine_, pcand_.get(), p1cand_.get() );

        prevLine_=line.get();

        Q_EMIT updateActors();

        // continue with next line
        p1_=p2_;
        p1cand_=std::move(pcand_);
        p2_.reset();
        return true;
    }

    pcand_.reset();

    return false;
}


bool IQVTKCADModel3DViewerDrawLine::onRightButtonDown(
        Qt::KeyboardModifiers nFlags,
        const QPoint point )
{
    finishAction();
    Q_EMIT finished();
    return true;
}
