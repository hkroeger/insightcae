#include "iqvtkcadmodel3dviewerdrawarc.h"
#include "base/units.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkconstrainedsketcheditor.h"

#include "datum.h"
#include "cadfeatures/line.h"


#include "vtkMapper.h"
#include "vtkArcSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkCamera.h"
#include <iostream>
#include <qnamespace.h>



using namespace insight;
using namespace insight::cad;




void IQVTKCADModel3DViewerDrawArc::updatePreviewLine(const arma::mat& pip3d)
{
    if (p1_)
    {
        if (!previewLine_)
        {
            // create line
            auto arc = vtkSmartPointer<vtkArcSource>::New();
            arc->SetResolution(25);
            arc->SetUseNormalAndAngle(true);

            previewLine_ = vtkSmartPointer<vtkActor>::New();
            previewLine_->SetMapper(
                vtkSmartPointer<vtkPolyDataMapper>::New() );
            previewLine_->GetMapper()->SetInputConnection(
                arc->GetOutputPort() );
            previewLine_->GetProperty()->SetColor(1, 0, 0);
            previewLine_->GetProperty()->SetLineWidth(2);
            viewer().renderer()->AddActor(previewLine_);
        }

        // update preview line
        if (!p2_ && !pm_) // draw as full circle
        {
            if (auto *arc = vtkArcSource::SafeDownCast(
                    previewLine_->GetMapper()->GetInputAlgorithm()))
            {
                arma::mat p2 = pip3d;
                arma::mat ctr = 0.5*( p1_->p->value() + p2 );
                arma::mat r = p2-ctr;

                arc->SetCenter(ctr.memptr());
                arc->SetNormal(sketch().sketchPlaneNormal()->value().memptr());
                arc->SetPolarVector(r.memptr());
                arc->SetAngle(360);

                previewLine_->GetMapper()->Update();

                viewer().scheduleRedraw();
            }
        }
        else if (p2_ && !pm_) // draw actual full preview
        {
            if (auto *arc = vtkArcSource::SafeDownCast(
                    previewLine_->GetMapper()->GetInputAlgorithm()))
            {
                if (arma::norm(p1_->p->value()-pip3d,2)>SMALL
                    && arma::norm(p2_->p->value()-pip3d,2)>SMALL)
                {
                    auto crv = GC_MakeArcOfCircle(
                        to_Pnt(p1_->p->value()),
                        to_Pnt(pip3d),
                        to_Pnt(p2_->p->value())
                    ).Value();

                    auto u=crv->LastParameter()-crv->FirstParameter();

                    auto c = Handle_Geom_Circle::DownCast(crv->BasisCurve());
                    arma::mat ctr = vec3(c->Circ().Position().Location());
                    auto n = vec3(c->Circ().Axis().Direction());
                    arma::mat r = vec3(crv->Value(crv->FirstParameter()))-ctr;
                    double R=arma::norm(r,2);

                    std::cout<<ctr.t()<<n.t()<<r.t()<<R<<" u="<<u;

                    if (R>SMALL)
                    {
                        double ang=(u)*180./M_PI;

                        std::cout<<" ang="<<ang;

                        arc->SetCenter(ctr.memptr());
                        arc->SetNormal(n.memptr());
                        arc->SetPolarVector(r.memptr());
                        arc->SetAngle(ang);

                        previewLine_->GetMapper()->Update();

                        viewer().scheduleRedraw();
                    }
                }
            }
        }
    }
}





IQVTKCADModel3DViewerPlanePointBasedAction::PointProperty
IQVTKCADModel3DViewerDrawArc::applyWizards(
    const arma::mat& pip3d,
    insight::cad::FeaturePtr onFeature ) const
{
    arma::mat pip=pip3d;
    arma::mat p22 =
        viewer().pointInPlane2D(
            sketch().plane()->plane(), pip);

    // if (p1_)
    // {
    //     arma::mat p21 =
    //         viewer().pointInPlane2D(
    //             sketch().plane()->plane(), p1_->p->value());

    //     arma::mat l = p22 - p21;
    //     double angle = atan2(l(1), l(0));

    //     const double angleGrid=22.5*SI::deg, angleCatch=5.*SI::deg;
    //     for (double a=-180.*SI::deg; a<180.*SI::deg; a+=angleGrid)
    //     {
    //         if (fabs(angle-a)<angleCatch)
    //         {
    //             pip=viewer().pointInPlane3D(
    //                 sketch().plane()->plane(),
    //                 p21+vec2(cos(a),
    //                            sin(a))*arma::norm(l,2) );
    //             break;
    //         }
    //     }

    //     p22=viewer().pointInPlane2D(sketch().plane()->plane(), pip);
    // }

    return {
            SketchPoint::create(
                sketch().plane(),
                p22(0), p22(1) ), false, nullptr};
}





IQVTKCADModel3DViewerDrawArc::IQVTKCADModel3DViewerDrawArc(
    IQVTKConstrainedSketchEditor &editor )
: IQVTKCADModel3DViewerPlanePointBasedAction(editor),
    previewLine_(nullptr)
{}




IQVTKCADModel3DViewerDrawArc::~IQVTKCADModel3DViewerDrawArc()
{
    if (previewLine_)
    {
        previewLine_->SetVisibility(false);
        viewer().renderer()->RemoveActor(previewLine_);
    }
}

void IQVTKCADModel3DViewerDrawArc::start()
{
    pointSelected.connect(
        [this](PointProperty pp)
        {
            DBG_SLOT(pointSelected);

            if (!p1_)
            {
                p1_=std::make_shared_aggr<PointProperty>(pp);

                if ( !pp.isAnExistingPoint )
                {
                    sketch().insertGeometry(
                        std::dynamic_pointer_cast
                        <ConstrainedSketchEntity>( p1_->p ) );
                }
                // Q_EMIT endPointSelected(p1_.get(), nullptr);

                sketch().invalidate();
            }
            else if (!p2_)
            {
                if (arma::norm(pp.p->coords2D() - p1_->p->coords2D(),2)>SMALL)
                {
                    p2_=std::make_shared_aggr<PointProperty>(pp);

                    if ( !pp.isAnExistingPoint )
                    {
                        sketch().insertGeometry(
                            std::dynamic_pointer_cast
                            <ConstrainedSketchEntity>( p2_->p ) );
                    }
                    // Q_EMIT endPointSelected(p2_.get(), p1_->p);

                    sketch().invalidate();
                }
            }
            else
            {
                if ( (arma::norm(pp.p->coords2D() - p1_->p->coords2D(),2)>SMALL)
                    && (arma::norm(pp.p->coords2D() - p2_->p->coords2D(),2)>SMALL) )
                {
                    pm_ = std::make_shared_aggr<PointProperty>(pp);

                    // Q_EMIT endPointSelected(pm_.get(), p2_->p);

                    auto arc = Arc::create(p1_->p, Arc::IntermediatePoint, pp.p, p2_->p);

                    auto crv=arc->calcArc();
                    auto circ = Handle_Geom_Circle::DownCast(crv->BasisCurve())->Circ();

                    bool reversed = arma::dot(
                        vec3(circ.Axis().Direction()),
                        sketch().sketchPlaneNormal()->value() ) < 0.;
                    std::cout<<"reversed="<<reversed<<std::endl;

                    auto midp=ArcCenterPoint::create(
                        sketch().plane(),
                        (crv->LastParameter()-crv->FirstParameter())
                        );

                    arc = Arc::create(
                        reversed ? p2_->p : p1_->p,
                        Arc::NormalTimesRadius,
                        midp,
                        reversed ? p1_->p : p2_->p);

                    sketch().insertGeometry(midp); // after creation of arc => sets link
                    sketch().insertGeometry(arc);
                    sketch().invalidate();

                    // Q_EMIT lineAdded(line, prevLine_, p2_.get(), p1_.get() );


                    // end line here, restart
                    previewLine_->SetVisibility(false);
                    viewer().renderer()->RemoveActor(previewLine_);
                    previewLine_=nullptr;
                    p1_.reset();
                    p2_.reset();
                    pm_.reset();
                }
            }
        }
        );

    newPreviewEntity.connect(
        [this](std::weak_ptr<insight::cad::ConstrainedSketchEntity> pP)
        {
            DBG_SLOT(newPreviewEntity);
            if (auto p =
                std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                    pP.lock() ) )
            {
                updatePreviewLine(p->value());
            }
        }
        );
}




bool IQVTKCADModel3DViewerDrawArc::onMouseMove(
    const QPoint point,
    Qt::KeyboardModifiers curFlags )
{
    updatePreviewLine(
        IQVTKCADModel3DViewerPlanePointBasedAction::applyWizards(point, nullptr)
            .p->value() );

    return IQVTKCADModel3DViewerPlanePointBasedAction
        ::onMouseMove(point, curFlags);
}





bool IQVTKCADModel3DViewerDrawArc::onMouseClick  (
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



QString IQVTKCADModel3DViewerDrawArc::description() const
{
    return "Draw arc";
}
