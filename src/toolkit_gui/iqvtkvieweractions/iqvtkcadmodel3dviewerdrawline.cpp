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




arma::mat IQVTKCADModel3DViewerPlanePointBasedAction::pointInPlane3D(const arma::mat &pip2d) const
{
    insight::assertion(
                pip2d.n_elem==2,
                "expected 2D vector, got %d", pip2d.n_elem );

    auto plane=sketch_->plane()->plane();
    gp_Trsf pl;
    pl.SetTransformation(plane); // from global to plane
    gp_Pnt p2(pip2d(0), pip2d(1), 0);
    auto pp = p2.Transformed(pl.Inverted());
    return vec3(pp);
}

arma::mat
IQVTKCADModel3DViewerPlanePointBasedAction::pointInPlane3D(
        const QPoint &screenPos ) const
{
    auto *renderer = viewer().renderer();
    auto v = viewer().widgetCoordsToVTK(screenPos);

    arma::mat p0=vec3(sketch_->plane()->plane().Location());
    arma::mat n=vec3(sketch_->plane()->plane().Direction());

    arma::mat l0=vec3Zero();
    renderer->SetDisplayPoint(v.x(), v.y(), 0.0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(l0.memptr());

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




arma::mat
IQVTKCADModel3DViewerPlanePointBasedAction::pointInPlane2D(const QPoint &screenPos) const
{
    return pointInPlane2D(pointInPlane3D(screenPos));
}




arma::mat
IQVTKCADModel3DViewerPlanePointBasedAction::pointInPlane2D(const arma::mat &p3) const
{
    insight::assertion(
                p3.n_elem==3,
                "expected 3D vector, got %d", p3.n_elem );

    auto plane=sketch_->plane()->plane();
    gp_Trsf pl;
    pl.SetTransformation(plane); // from global to plane
    auto pp = toVec<gp_Pnt>(p3).Transformed(pl);
    insight::assertion(
                fabs(pp.Z())<SMALL,
                "point (%g, %g, %g) not in plane with p=(%g, %g, %g) and n=(%g, %g, %g)!\n"
                "(local coordinates = (%g, %g, %g))",
                p3(0), p3(1), p3(2),
                plane.Location().X(), plane.Location().Y(), plane.Location().Z(),
                plane.Direction().X(), plane.Direction().Y(), plane.Direction().Z(),
                pp.X(), pp.Y(), pp.Z() );
    return vec2( pp.X(), pp.Y() );
}


SketchPointPtr
IQVTKCADModel3DViewerPlanePointBasedAction
::existingSketchPointAt( const QPoint& cp ) const
{
    if (auto act =
        viewer().findActorUnderCursorAt(cp))
    {
        if (auto se =
            std::dynamic_pointer_cast<IQVTKConstrainedSketchEditor>(
                viewer().currentUserActivity_))
        {
            if (auto sg =
                se->findSketchElementOfActor(act))
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
    CandidatePoint pc{nullptr, false, false};
    pc.sketchPoint = existingSketchPointAt(point);

    if (!pc.sketchPoint)
    {
        arma::mat pip=pointInPlane3D(point);

        if (p1_ && !p2_)
        {
            if (!pc.sketchPoint) // not over existing point
            {

                arma::mat p21=pointInPlane2D(p1_->value());
                arma::mat p22=pointInPlane2D(pip);

                arma::mat l = p22 - p21;
                double angle = atan2(l(1), l(0));

                const double angleGrid=22.5*SI::deg, angleCatch=5.*SI::deg;
                for (double a=-180.*SI::deg; a<180.*SI::deg; a+=angleGrid)
                {
                    if (fabs(angle-a)<angleCatch)
                    {
                        pip=pointInPlane3D( p21+vec2(cos(a), sin(a))*arma::norm(l,2) );
                        break;
                    }
                }

            }
        }

        auto p2=pointInPlane2D(pip);
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

            sketch_->geometry().insert(
                     std::make_shared
                        <IQVTKFixedPoint>( p1_ ) ); // fix first point
        }

        sketch_->invalidate();
        Q_EMIT updateActors();
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

        auto line = std::dynamic_pointer_cast<insight::cad::Line>(
                    Line::create(p1_, p2_) );
        sketch_->geometry().insert(line);
        sketch_->invalidate();

        Q_EMIT lineAdded(line.get(), prevLine_, pcand_->isAnExistingPoint);

        prevLine_=line.get();

        Q_EMIT updateActors();

        // continue with next line
        p1_=p2_;
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
    setFinished();
    Q_EMIT finished();
    return true;
}
