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
    IQVTKCADModel3DViewer &viewWidget,
    const insight::CoordinateSystem& cs,
    bool showOnlyX )
  : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget),
    cs_(cs),
    showOnlyX_(showOnlyX)
{
}

IQVTKManipulateCoordinateSystem::~IQVTKManipulateCoordinateSystem()
{
    viewer().renderer()->RemoveActor(aP0_);
    viewer().renderer()->RemoveActor(aEx_);
    if (!showOnlyX_)
    {
        viewer().renderer()->RemoveActor(aEy_);
        viewer().renderer()->RemoveActor(aEz_);
    }
    viewer().renderer()->RemoveActor(aXY_);
    viewer().renderer()->RemoveActor(aYZ_);
    viewer().renderer()->RemoveActor(aXZ_);
    viewer().scheduleRedraw();
}

void IQVTKManipulateCoordinateSystem::setCS()
{
    ctr_->SetCenter(cs_.origin.memptr());
    ctr_->Update();

    arma::mat origin = insight::vec3Zero();
    {
        auto tex=vtkTransform::SafeDownCast(ex_->GetTransform());
        auto matrixx=vtkSmartPointer<vtkMatrix4x4>::New();
        insight::CoordinateSystem(origin, cs_.ex).setVTKMatrix(matrixx);
        tex->SetMatrix(matrixx);
        tex->Scale(5., 5., 5.);
    }
    {
        auto tey=vtkTransform::SafeDownCast(ey_->GetTransform());
        auto matrixy=vtkSmartPointer<vtkMatrix4x4>::New();
        insight::CoordinateSystem(origin, cs_.ey).setVTKMatrix(matrixy);
        tey->SetMatrix(matrixy);
        tey->Scale(5., 5., 5.);
    }
    {
        auto tez=vtkTransform::SafeDownCast(ez_->GetTransform());
        auto matrixz=vtkSmartPointer<vtkMatrix4x4>::New();
        insight::CoordinateSystem(origin, cs_.ez).setVTKMatrix(matrixz);
        tez->SetMatrix(matrixz);
        tez->Scale(5., 5., 5.);
    }

    viewer().scheduleRedraw();
}

void IQVTKManipulateCoordinateSystem::start()
{
    ctr_ = vtkSmartPointer<vtkPointSource>::New();
    ctr_->SetNumberOfPoints(1);
    // ctr_->SetCenter(cs_.origin.memptr());
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

    aP0_=display(fixedCtr, insight::vec3(0.5,0.5,0.5));

    {
        auto a=arrow(ex_, pyz_);
        aEx_=display(a.first, insight::vec3(0,1,0));
        aYZ_=display(a.second, insight::vec3(0,1,0));
    }
    {
        auto a=arrow(ey_, pxz_);
        if (!showOnlyX_) aEy_=display(a.first, insight::vec3(1,0,0));
        aXZ_=display(a.second, insight::vec3(1,0,0));
    }
    {
        auto a=arrow(ez_, pxy_);
        if (!showOnlyX_) aEz_=display(a.first, insight::vec3(0,0,1));
        aXY_=display(a.second, insight::vec3(0,0,1));
    }

    setCS();
}



void IQVTKManipulateCoordinateSystem::ManipPlane::update(const QPoint& newp)
{
    if (start)
    {
        try
        {
            arma::mat n=normal();

            gp_Ax3 viewPlane(
                to_Pnt(parent->cs_.origin),
                to_Vec(n)
                );

            auto p0 = insight::normalized(
                parent->viewer().pointInPlane3D(
                    viewPlane,
                    *start
                    )
                );

            auto p1 = insight::normalized(
                parent->viewer().pointInPlane3D(
                    viewPlane,
                    newp
                    )
                );
            arma::mat dx=p1-p0;
            double ang=::acos(arma::dot(p0, p1));
            if (arma::dot(dx, arma::cross(p0,n))>0.)
                ang*=-1.;

            std::cout<<"ang="<<ang<<std::endl;

            parent->cs_.rotate(ang, n);

            parent->setCS();

            parent->viewer().scheduleRedraw();
        }
        catch (...)
        {
            // ignore errors, when no unique intersection with plane
        }
    }
    start=newp;
}

// void IQVTKManipulateCoordinateSystem::ManipEX::update(const QPoint& newp)
// {
//     vtkCamera* camera = parent->viewer().renderer()->GetActiveCamera();

//     auto n=insight::vec3FromComponents(camera->GetViewPlaneNormal());
//     auto ev = insight::normalized(arma::cross(n, parent->cs_.ex));
//     arma::mat en = insight::normalized(arma::cross(parent->cs_.ex, ev));

//     // goes through origin and ex
//     gp_Ax3 viewPlane(
//         to_Pnt(parent->cs_.origin),
//         to_Vec(en)
//         );

//     auto nex = insight::normalized(
//         parent->viewer().pointInPlane3D(
//             viewPlane,
//             newp
//         )
//     );

//     // rotate EX in
//     parent->cs_=insight::CoordinateSystem(
//         parent->cs_.origin, nex, parent->cs_.ez);
//     parent->setCS();

//     parent->viewer().scheduleRedraw();
// }

// void IQVTKManipulateCoordinateSystem::ManipEY::update(const QPoint& newp)
// {
//     vtkCamera* camera = parent->viewer().renderer()->GetActiveCamera();

//     auto n=insight::vec3FromComponents(camera->GetViewPlaneNormal());
//     auto ev = insight::normalized(arma::cross(n, parent->cs_.ey));
//     arma::mat en = insight::normalized(arma::cross(parent->cs_.ey, ev));

//     // goes through origin and ex
//     gp_Ax3 viewPlane(
//         to_Pnt(parent->cs_.origin),
//         to_Vec(en)
//         );

//     auto ney = insight::normalized(
//         parent->viewer().pointInPlane3D(
//             viewPlane,
//             newp
//             )
//         );

//     parent->cs_=insight::CoordinateSystem(
//         parent->cs_.origin, arma::cross(ney, parent->cs_.ez), parent->cs_.ez);
//     parent->setCS();

//     parent->viewer().scheduleRedraw();
// }

// void IQVTKManipulateCoordinateSystem::ManipEZ::update(const QPoint& newp)
// {
//     vtkCamera* camera = parent->viewer().renderer()->GetActiveCamera();

//     auto n=insight::vec3FromComponents(camera->GetViewPlaneNormal());
//     auto ev = insight::normalized(arma::cross(n, parent->cs_.ez));
//     arma::mat en = insight::normalized(arma::cross(parent->cs_.ez, ev));

//     // goes through origin and ex
//     gp_Ax3 viewPlane(
//         to_Pnt(parent->cs_.origin),
//         to_Vec(en)
//         );

//     auto nez = insight::normalized(
//         parent->viewer().pointInPlane3D(
//             viewPlane,
//             newp
//             )
//         );

//     parent->cs_=insight::CoordinateSystem(
//         parent->cs_.origin, arma::cross(parent->cs_.ey, nez), nez);
//     parent->setCS();

//     parent->viewer().scheduleRedraw();
// }

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

bool IQVTKManipulateCoordinateSystem::onLeftButtonDown(
    Qt::KeyboardModifiers nFlags,
    const QPoint point, bool afterDoubleClick )
{
    if (auto hit = viewer().findActorUnderCursorAt(
            point, {aXY_, aYZ_, aXZ_/*aP0_,*/ /*aEx_, aEy_, aEz_*/} ))
    {
        if (hit==aXY_)
        {
            auto mex=std::make_unique<ManipPlane>();
            mex->parent=this;
            mex->normal=[&]() { return cs_.ez; };
            action=std::move(mex);
            return true;
        }
        else if (hit==aYZ_)
        {
            auto mex=std::make_unique<ManipPlane>();
            mex->parent=this;
            mex->normal=[&]() { return cs_.ex; };
            action=std::move(mex);
            return true;
        }
        else if (hit==aXZ_)
        {
            auto mex=std::make_unique<ManipPlane>();
            mex->parent=this;
            mex->normal=[&]() { return cs_.ey; };
            action=std::move(mex);
            return true;
        }
        // if (hit==aEx_)
        // {
        //     auto mex=std::make_unique<ManipEX>();
        //     mex->start=point;
        //     mex->parent=this;
        //     action=std::move(mex);
        //     return true;
        // }
        // else if (hit==aEy_)
        // {
        //     auto mey=std::make_unique<ManipEY>();
        //     mey->start=point;
        //     mey->parent=this;
        //     action=std::move(mey);
        //     return true;
        // }
        // else if (hit==aEz_)
        // {
        //     auto mez=std::make_unique<ManipEZ>();
        //     mez->start=point;
        //     mez->parent=this;
        //     action=std::move(mez);
        //     return true;
        // }
        // else if (hit==aP0_)
        // {
        //     auto mp0=std::make_unique<ManipP0>();
        //     mp0->start=point;
        //     mp0->parent=this;
        //     action=std::move(mp0);
        //     return true;
        // }
    }

    return false;
}

bool IQVTKManipulateCoordinateSystem::onMouseMove(
    Qt::MouseButtons buttons,
    const QPoint point,
    Qt::KeyboardModifiers curFlags )
{
    if (action)
    {
        action->update(point);
        return true;
    }

    return false;
}


bool IQVTKManipulateCoordinateSystem::onLeftButtonUp(
    Qt::KeyboardModifiers nFlags,
    const QPoint point,
    bool lastClickWasDoubleClick )
{
    if (action)
    {
        action.reset();
        return true;
    }

    return false;
}

bool IQVTKManipulateCoordinateSystem::onRightButtonDown(
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    this->finishAction(false);
    return true;
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
