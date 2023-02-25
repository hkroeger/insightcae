#include "iqvtkcadmodel3dviewerdrawline.h"
#include "iqvtkcadmodel3dviewer.h"

#include "cadfeatures/line.h"
#include "datum.h"

#include "vtkMapper.h"
#include "vtkLineSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"



using namespace insight;
using namespace insight::cad;




arma::mat
IQVTKCADModel3DViewerPlanePointBasedAction::pointInPlane(
        const QPoint &screenPos ) const
{
    auto *renderer = viewer().renderer();
    double vx=screenPos.x();
    double vy=viewer().size().height()-screenPos.y();

    insight::dbg()<<"vx="<<vx<<", vy="<<vy<<std::endl;

    arma::mat p0=vec3(sketch_->plane()->plane().Location());
    arma::mat n=vec3(sketch_->plane()->plane().Direction());

    arma::mat l0=vec3Zero();
    renderer->SetDisplayPoint(vx, vy, 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(l0.memptr());
    insight::dbg()<<"lx="<<l0(0)<<", ly="<<l0(1)<<", lz="<<l0(2)<<std::endl;

    arma::mat camPos, camFocal;
    camPos=camFocal=vec3Zero();
    renderer->GetActiveCamera()->GetPosition(camPos.memptr());
    renderer->GetActiveCamera()->GetFocalPoint(camFocal.memptr());
    arma::mat l = normalized(camFocal-camPos);

    double nom=arma::dot((p0-l0),n);
    insight::assertion(
                fabs(nom)>SMALL,
                "no single intersection" );

    double denom=arma::dot(l,n);
    insight::assertion(
                fabs(denom)>SMALL,
                "no intersection" );

    double d=nom/denom;

    return l0+l*d;
}


SketchPointPtr
IQVTKCADModel3DViewerPlanePointBasedAction
::sketchPointAtCursor(
        const QPoint& cp ) const
{
    if (auto act =
            viewer().findActorUnderCursorAt(cp))
    {
        if (auto sg =
                viewer().displayedSketch_
                ->findSketchElementOfActor(act))
        {
            if (auto sp =
                    std::dynamic_pointer_cast
                     <SketchPoint>(sg))
            {
                return sp;
            }
        }
    }

    return std::make_shared<SketchPoint>(
                sketch_->plane(),
                pointInPlane(cp) );
}


IQVTKCADModel3DViewerPlanePointBasedAction
::IQVTKCADModel3DViewerPlanePointBasedAction(
        IQVTKCADModel3DViewer &viewWidget,
        insight::cad::ConstrainedSketchPtr sketch )
    : ViewWidgetAction<IQVTKCADModel3DViewer>(
          viewWidget),
      sketch_(sketch)
{}




IQVTKCADModel3DViewerDrawLine
::IQVTKCADModel3DViewerDrawLine(
        IQVTKCADModel3DViewer &viewWidget,
        insight::cad::ConstrainedSketchPtr sketch )
    : IQVTKCADModel3DViewerPlanePointBasedAction(
          viewWidget, sketch),
      previewLine_(nullptr)
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
    if (p1_ && !p2_)
    {
        if (previewLine_)
        {
            if (auto*line = vtkLineSource::SafeDownCast(
                previewLine_->GetMapper()->GetInputAlgorithm()))
            {
                line->SetPoint1(p1_->value().memptr());
                line->SetPoint2(pointInPlane(point).memptr());
                previewLine_->GetMapper()->Update();
            }
        }
        else
        {
            auto l = vtkSmartPointer<vtkLineSource>::New();
            l->SetPoint1(p1_->value().memptr());
            auto p2 = pointInPlane(point);
            l->SetPoint2(p2.memptr());

            previewLine_ = vtkSmartPointer<vtkActor>::New();
            previewLine_->SetMapper(vtkSmartPointer<vtkPolyDataMapper>::New());
            previewLine_->GetMapper()->SetInputConnection(l->GetOutputPort());
            previewLine_->GetProperty()->SetColor(1, 0, 0);
            previewLine_->GetProperty()->SetLineWidth(2);
            viewer().renderer()->AddActor(previewLine_);
        }
    }
}





void IQVTKCADModel3DViewerDrawLine::onLeftButtonUp(
        Qt::KeyboardModifiers nFlags,
        const QPoint point )
{
    if (!p1_)
    {
        p1_=sketchPointAtCursor(point);

        sketch_->geometry().insert(
                 std::dynamic_pointer_cast
                    <ConstrainedSketchGeometry>( p1_ ) );
        sketch_->invalidate();
        Q_EMIT updateActors();
    }
    else if (!p2_)
    {
        p2_=sketchPointAtCursor(point);

        sketch_->geometry().insert(
                 std::dynamic_pointer_cast
                    <ConstrainedSketchGeometry>( p2_ ) );
        sketch_->geometry().insert(
                    std::dynamic_pointer_cast
                    <ConstrainedSketchGeometry>(
                        Line::create(p1_, p2_) ) );
        sketch_->invalidate();

        Q_EMIT updateActors();

        // continue with next line
        p1_=p2_;
        p2_.reset();
    }
}


void IQVTKCADModel3DViewerDrawLine::onRightButtonUp(
        Qt::KeyboardModifiers nFlags,
        const QPoint point )
{
    setFinished();
    Q_EMIT finished();
}
