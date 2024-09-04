#include "iqvtkviewer.h"

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"

#include <QMouseEvent>
#include <QInputDialog>


IQVTKViewer::IQVTKViewer(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags),
      vtkWidget_(this)
{
    setCentralWidget(&vtkWidget_);

    setMouseTracking( true );
    setFocusPolicy(Qt::StrongFocus);

    ren_ = vtkSmartPointer<vtkRenderer>::New();
    renWin()->AddRenderer(ren_);

    ren_->GetActiveCamera()->SetParallelProjection(true);
    ren_->ResetCamera();
    insight::dbg()<<"viewer constructed"<<std::endl;
}


static const double DevicePixelRatioTolerance = 1e-5;

QPointF IQVTKViewer::widgetCoordsToVTK(const QPoint &widgetCoords) const
{
    auto pw = vtkWidget_.mapFromParent(widgetCoords);
    double DevicePixelRatio=vtkWidget_.devicePixelRatioF();
    QPoint p(
        static_cast<int>(pw.x() * DevicePixelRatio + DevicePixelRatioTolerance),
        static_cast<int>(pw.y() * DevicePixelRatio + DevicePixelRatioTolerance)
        );
    return QPointF(
        p.x(),
        vtkWidget_.size().height()-p.y()-1
        );
}

arma::mat IQVTKViewer::pointInPlane3D(
    const arma::mat &planeNormal,
    const arma::mat &p0,
    const QPoint &screenPos) const
{
    insight::CurrentExceptionContext ex("mapping screen position to 3D position in plane (point and normal)");

    using namespace insight;

    auto *renderer = const_cast<IQVTKViewer*>(this)->renderer();
    auto v = widgetCoordsToVTK(screenPos);

    arma::mat n=normalized(planeNormal);

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


vtkRenderWindowInteractor* IQVTKViewer::interactor()
{
    return vtkWidget_.
#if (VTK_MAJOR_VERSION>8) || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=3)
        interactor
#else
        GetInteractor
#endif
        ();
}


vtkRenderer* IQVTKViewer::renderer()
{
    return ren_;
}


vtkRenderWindow* IQVTKViewer::renWin()
{
    return vtkWidget_.
#if (VTK_MAJOR_VERSION>8) || (VTK_MAJOR_VERSION==8 && VTK_MINOR_VERSION>=3)
        renderWindow
#else
        GetRenderWindow
#endif
        ();
}




IQImageReferencePointSelectorWindow::IQImageReferencePointSelectorWindow(
    vtkImageActor *toBeDisplayed, QWidget *parent, Qt::WindowFlags flags)
    : IQVTKViewer(parent, flags),
    actor_(toBeDisplayed)
{
    renderer()->AddActor(actor_);

    actor_->GetInput()->GetOrigin(origin);
    actor_->GetInput()->GetSpacing(spacing);
    actor_->GetInput()->GetExtent(extent);

    setupCameraForImage(actor_->GetInput(), renderer()->GetActiveCamera());

    insight::dbg()<<"img ref viewer constructed"<<std::endl;

    p1_=std::make_shared<arma::mat>(insight::vec3(63.8, 724.1, 0));
    p2_=std::make_shared<arma::mat>(insight::vec3(63.8, 724.1, 0));
    p3_=std::make_shared<arma::mat>(insight::vec3(349.823, 722.1, 0));
    L_=50.;
}


void IQImageReferencePointSelectorWindow::setupCameraForImage(vtkImageData* imageData, vtkCamera* camera)
{
    double origin[3];
    double spacing[3];
    int extent[6];
    imageData->GetOrigin(origin);
    imageData->GetSpacing(spacing);
    imageData->GetExtent(extent);

    camera->ParallelProjectionOn();

    double xc = origin[0] + 0.5 * (extent[0] + extent[1]) * spacing[0];
    double yc = origin[1] + 0.5 * (extent[2] + extent[3]) * spacing[1];
    // double xd = (extent[1] - extent[0] + 1)*spacing[0];
    double yd = (extent[3] - extent[2] + 1) * spacing[1];
    double d = camera->GetDistance();
    camera->SetParallelScale(0.5 * yd);
    camera->SetFocalPoint(xc, yc, 0.0);
    camera->SetPosition(xc, yc, d);
}

void IQImageReferencePointSelectorWindow::mousePressEvent(QMouseEvent *e)
{
    if ( e->button() & Qt::LeftButton )
    {
        arma::mat p = pointInPlane3D(
            insight::vec3Z(), insight::vec3Zero(),
            e->pos() );

        auto sp = vtkSmartPointer<vtkSphereSource>::New();
        sp->SetCenter(p[0], p[1], p[2]);

        vtkNew<vtkPolyDataMapper> mapper;
        mapper->SetInputConnection(sp->GetOutputPort());

        vtkNew<vtkActor> actor;
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(1, 0, 0);
        renderer()->AddActor(actor);

        if (!p1_)
        {
            p1_=std::make_shared<arma::mat>(p);
        }
        else if (!p2_)
        {
            L_=QInputDialog::getDouble(this, "Enter Distance",
                                    "Please enter the distance to the next point, you will pick:");
            p2_=std::make_shared<arma::mat>(p);
        }
        else if (!p3_)
        {
            p2_=std::make_shared<arma::mat>(p);
            hide();
        }
    }
}
