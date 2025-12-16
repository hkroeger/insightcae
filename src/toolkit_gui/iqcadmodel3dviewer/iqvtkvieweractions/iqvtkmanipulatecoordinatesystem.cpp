#include "iqvtkmanipulatecoordinatesystem.h"
#include "base/linearalgebra.h"
#include "iqvtkcadmodel3dviewer.h"
#include <qnamespace.h>
#include <vtkSphere.h>
#include <vtkSmartPointer.h>
#include <vtkArrowSource.h>
#include <vtkDistanceToCamera.h>
#include <vtkGlyph3D.h>
#include <vtkTransform.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointSource.h>
#include <vtkSphereSource.h>
#include <vtkRegularPolygonSource.h>

IQVTKManipulateCoordinateSystem::IQVTKManipulateCoordinateSystem(
    ViewWidgetActionHost<IQVTKCADModel3DViewer> &parent,
    const insight::CoordinateSystem& cs,
    bool locPosition,
    ActorMask actorsToShow )
  : ViewWidgetAction<IQVTKCADModel3DViewer>(parent, false),
    cs_(cs),
    locPosition_(locPosition),
    actorsToShow_(actorsToShow)
{
    if (actorsToShow_.size()<1)
    {
        actorsToShow_={P0, Ex, Ey, Ez, XY, XZ, YZ};
    }
    aboutToBeDestroyed.connect(
        [this](){
            DBG_SLOT(aboutToBeDestroyed);

            for (auto& actor: actors_)
            {
                if (actor.second)
                    viewer().renderer()->RemoveActor(actor.second);
            }

            viewer().scheduleRedraw();
        });

}

QString IQVTKManipulateCoordinateSystem::description() const
{
    return "Manipulation coordinate system";
}


vtkActor *IQVTKManipulateCoordinateSystem::actorIfActive(Actor a) const
{
    auto i=actors_.find(a);
    if (i!=actors_.end())
        return i->second;
    else
        return nullptr;
}

void IQVTKManipulateCoordinateSystem::setCS()
{
    ctr_->SetCenter(cs_.origin.memptr());
    ctr_->Update();

    arma::mat origin = insight::vec3Zero();
    {
        auto tex=vtkTransform::SafeDownCast(ex_->GetTransform());
        auto matrixx=vtkSmartPointer<vtkMatrix4x4>::New();
        insight::CoordinateSystem(origin, cs_.ex, cs_.ez).setVTKMatrix(matrixx);
        tex->SetMatrix(matrixx);
        tex->Scale(5., 5., 5.);
    }
    {
        auto tey=vtkTransform::SafeDownCast(ey_->GetTransform());
        auto matrixy=vtkSmartPointer<vtkMatrix4x4>::New();
        insight::CoordinateSystem(origin, cs_.ey, cs_.ez).setVTKMatrix(matrixy);
        tey->SetMatrix(matrixy);
        tey->Scale(5., 5., 5.);
    }
    {
        auto tez=vtkTransform::SafeDownCast(ez_->GetTransform());
        auto matrixz=vtkSmartPointer<vtkMatrix4x4>::New();
        insight::CoordinateSystem(origin, cs_.ez, -cs_.ex).setVTKMatrix(matrixz);
        tez->SetMatrix(matrixz);
        tez->Scale(5., 5., 5.);
    }

    viewer().scheduleRedraw();
}

void IQVTKManipulateCoordinateSystem::start()
{
    ctr_ = vtkSmartPointer<vtkPointSource>::New();
    ctr_->SetNumberOfPoints(1);
    ctr_->SetRadius(0.);

    auto cdist = vtkSmartPointer<vtkDistanceToCamera>::New();
    cdist->SetInputConnection(ctr_->GetOutputPort());
    cdist->SetScreenSize(50);
    cdist->SetRenderer(viewer().renderer());

    // display actors
    auto unitsphere = vtkSmartPointer<vtkSphereSource>::New();
    auto sph = vtkSmartPointer<vtkGlyph3D>::New();
    sph->SetInputConnection(ctr_->GetOutputPort());
    sph->SetSourceConnection(unitsphere->GetOutputPort());
    sph->SetScaleFactor(1);


    auto arrow = [&]( vtkSmartPointer<vtkTransformFilter> & arr,
                       vtkSmartPointer<vtkTransformFilter> & disk )
    {
        auto unitarr = vtkSmartPointer<vtkArrowSource>::New();

        auto unitdisk = vtkSmartPointer<vtkRegularPolygonSource>::New();
        unitdisk->GeneratePolygonOff();
        unitdisk->SetNumberOfSides(50);
        unitdisk->SetNormal(1,0,0);
        unitdisk->SetRadius(1.);

        arr = vtkSmartPointer<vtkTransformFilter>::New();
        arr->SetInputConnection(unitarr->GetOutputPort());
        auto trsfarr=vtkSmartPointer<vtkTransform>::New();
        arr->SetTransform(trsfarr);

        disk = vtkSmartPointer<vtkTransformFilter>::New();
        disk->SetInputConnection(unitdisk->GetOutputPort());
        disk->SetTransform(trsfarr);

        auto fixedArrow = vtkSmartPointer<vtkGlyph3D>::New();
        fixedArrow->SetInputConnection(cdist->GetOutputPort());
        fixedArrow->SetSourceConnection(arr->GetOutputPort());
        fixedArrow->SetScaleModeToScaleByScalar();
        fixedArrow->SetInputArrayToProcess(
            0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");

        auto fixedDisk = vtkSmartPointer<vtkGlyph3D>::New();
        fixedDisk->SetInputConnection(cdist->GetOutputPort());
        fixedDisk->SetSourceConnection(disk->GetOutputPort());
        fixedDisk->SetScaleModeToScaleByScalar();
        fixedDisk->SetInputArrayToProcess(
            0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");

        return std::pair<
            vtkSmartPointer<vtkGlyph3D>,
            vtkSmartPointer<vtkGlyph3D> >
            { fixedArrow, fixedDisk };
    };

    auto display = [&](vtkAlgorithm* a, const arma::mat& c) {
        auto map=vtkSmartPointer<vtkPolyDataMapper>::New();
        map->SetInputConnection(a->GetOutputPort());
        map->SetScalarVisibility(false);
        auto act=vtkSmartPointer<vtkActor>::New();
        act->SetMapper(map);
        act->GetProperty()->SetColor(c(0), c(1), c(2));
        viewer().renderer()->AddActor(act);
        return act;
    };


    auto fixedCtr = vtkSmartPointer<vtkGlyph3D>::New();
    fixedCtr->SetInputConnection(cdist->GetOutputPort());
    fixedCtr->SetSourceConnection(sph->GetOutputPort());
    fixedCtr->SetScaleModeToScaleByScalar();
    fixedCtr->SetInputArrayToProcess(
        0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "DistanceToCamera");

    if (actorsToShow_.count(P0))
        actors_[P0]=display(fixedCtr, insight::vec3(0.5,0.5,0.5));

    {
        auto a=arrow(ex_, pyz_);

        if (actorsToShow_.count(Ex))
            actors_[Ex]=display(a.first, insight::vec3(1,0,0));

        if (actorsToShow_.count(YZ))
            actors_[YZ]=display(a.second, insight::vec3(1,0,0));
    }
    {
        auto a=arrow(ey_, pxz_);

        if (actorsToShow_.count(Ey))
            actors_[Ey]=display(a.first, insight::vec3(0,1,0));

        if (actorsToShow_.count(XZ))
            actors_[XZ]=display(a.second, insight::vec3(0,1,0));
    }
    {
        auto a=arrow(ez_, pxy_);

        if (actorsToShow_.count(Ez))
            actors_[Ez]=display(a.first, insight::vec3(0,0,1));

        if (actorsToShow_.count(XY))
            actors_[XY]=display(a.second, insight::vec3(0,0,1));
    }

    setCS();
}


bool IQVTKManipulateCoordinateSystem::onMouseMove(
    const QPoint point,
    Qt::KeyboardModifiers curFlags )
{
    if (!ViewWidgetAction::onMouseMove(point, curFlags))
    {
        std::set eligible{XY, XZ, YZ};
        if (!locPosition_)
            eligible.insert({Ex, Ey, Ez});

        if (auto hit = viewer().findActorUnderCursorAt(
                point,
                std::container_type_cast<std::set<vtkProp*> >(
                    map_items(actors_)) ))
        {

            for (auto& a: eligible)
            {
                auto ta=actorIfActive(a);
                if (ta==hit)
                {
                    std::cout<<"hit "<<a<<std::endl;
                    viewer().highlightActor(ta);
                }
            }
            std::cout<<"hit check done"<<std::endl;
        }
    }
    return false;
}



void IQVTKManipulateCoordinateSystem::ManipPlane::update(const QPoint& newp)
{
    try
    {
        arma::mat n=insight::normalized(normal());

        auto p1 =
            parent->viewer().pointInPlane3D(
                gp_Ax3(
                    to_Pnt(parent->cs_.origin),
                    to_Vec(n)
                    ),
                newp
                );

        if (startPt)
        {
            arma::mat dx=p1-*startPt;
            if (arma::norm(dx,2) > insight::SMALL)
            {
                double ang=insight::rotAngle(
                    p1 - parent->cs_.origin,
                    *startPt - parent->cs_.origin,
                    n );

                insight::assertion(
                    !std::isnan(ang),
                    "invalid rotation angle");

                parent->cs_.rotate(ang, n);

                parent->setCS();

                parent->viewer().scheduleRedraw();
            }

        }
        startPt=p1;
    }
    catch (...)
    {
        // ignore errors, when no unique intersection with plane
    }
}


arma::mat IQVTKManipulateCoordinateSystem::ManipAlong::closestPt(const QPoint &newp)
{
    vtkCamera* camera = parent->viewer().renderer()->GetActiveCamera();

    auto d1=insight::normalized(
        insight::vec3FromComponents(
            camera->GetViewPlaneNormal()));

    auto p1 = parent->viewer().pointInPlane3D(
            gp_Ax3( // plane orthogonal to view direction and through CS origin
                to_Pnt(parent->cs_.origin),
                to_Vec(d1)
                ),
            newp
        );

    arma::mat r = p1 - p2;

    double a = dot(d1, d1);
    double b = dot(d1, d2);
    double c = dot(d2, d2);
    double d = dot(d1, r);
    double e = dot(d2, r);

    double denom = a*c - b*b;

    insight::assertion(
        std::abs(denom) > insight::SMALL,
        "view direction parallel to direction");

    double s = (a*e - b*d) / denom;

    // arma::mat point1 = p1 + d1 * t;
    arma::mat point2 = p2 + d2 * s;

    return point2;
}


void IQVTKManipulateCoordinateSystem::ManipAlong::update(const QPoint& newp)
{
    arma::mat curp=closestPt(newp);

    if (!startDiff_)
    {
        startDiff_=parent->cs_.origin-curp;
    }
    else
    {
        parent->cs_=insight::CoordinateSystem(
            *startDiff_+curp, parent->cs_.ex, parent->cs_.ez);
        parent->setCS();

        parent->viewer().scheduleRedraw();
    }
}




void IQVTKManipulateCoordinateSystem::ManipP0::update(const QPoint& newp)
{
    vtkCamera* camera = parent->viewer().renderer()->GetActiveCamera();

    auto n=insight::vec3FromComponents(camera->GetViewPlaneNormal());

    // goes through origin and ex
    gp_Ax3 viewPlane(
        to_Pnt(parent->cs_.origin),
        to_Vec(n)
        );

    auto np0 = insight::normalized(
        parent->viewer().pointInPlane3D(
            viewPlane,
            newp
            )
        );

    parent->cs_=insight::CoordinateSystem(
        np0, parent->cs_.ex, parent->cs_.ez);
    parent->setCS();

    parent->viewer().scheduleRedraw();
}




bool IQVTKManipulateCoordinateSystem::onMouseDrag(
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point,
    EventType eventType )
{
    if ((eventType==Begin) && !action )
    {
        if (auto hit = viewer().findActorUnderCursorAt(
                point,
                std::container_type_cast<std::set<vtkProp*> >(
                    map_items(actors_)) ))
        {
            if (hit==actorIfActive(XY))
            {
                auto mex=std::make_unique<ManipPlane>();
                mex->parent=this;
                mex->normal=[&]() { return cs_.ez; };
                action=std::move(mex);
                return true;
            }
            else if (hit==actorIfActive(YZ))
            {
                auto mex=std::make_unique<ManipPlane>();
                mex->parent=this;
                mex->normal=[&]() { return cs_.ex; };
                action=std::move(mex);
                return true;
            }
            else if (hit==actorIfActive(XZ))
            {
                auto mex=std::make_unique<ManipPlane>();
                mex->parent=this;
                mex->normal=[&]() { return cs_.ey; };
                action=std::move(mex);
                return true;
            }
            else if (!locPosition_ && (hit==actorIfActive(Ex)))
            {
                auto mex=std::make_unique<ManipAlong>();
                mex->parent=this;
                mex->p2=cs_.origin;
                mex->d2=cs_.ex;
                action=std::move(mex);
                return true;
            }
            else if (!locPosition_ && (hit==actorIfActive(Ey)))
            {
                auto mex=std::make_unique<ManipAlong>();
                mex->parent=this;
                mex->p2=cs_.origin;
                mex->d2=cs_.ey;
                action=std::move(mex);
                return true;
            }
            else if (!locPosition_ && (hit==actorIfActive(Ez)))
            {
                auto mex=std::make_unique<ManipAlong>();
                mex->parent=this;
                mex->p2=cs_.origin;
                mex->d2=cs_.ez;
                action=std::move(mex);
                return true;
            }
        }
    }
    else if (action)
    {
        if (eventType==Intermediate)
        {

            action->update(point);
            return true;
        }
        else if (eventType==Final)
        {
            action.reset();
            return true;
        }
    }

    return ViewWidgetAction::onMouseDrag(
        btn, nFlags, point, eventType);
}



bool IQVTKManipulateCoordinateSystem::onMouseClick(
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point)
{
    if (btn&Qt::RightButton)
    {
        this->finishAction(false);
        return true;
    }

    return
        ViewWidgetAction::onMouseClick(
            btn, nFlags, point );
}




bool IQVTKManipulateCoordinateSystem::onKeyPress ( Qt::KeyboardModifiers modifiers, int key )
{
    if ( (key==Qt::Key_Enter) || (key==Qt::Key_Return) )
    {
        Q_EMIT coordinateSystemSelected(cs_);
        finishAction(true);
        return true;
    }
    return false;
}
