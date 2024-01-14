#include "orientbackgroundimage.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkImageActor.h"
#include "vtkPointSource.h"

#include "orientbackgroundimagecoordinatesdialog.h"

void IQVTKOrientBackgroundImage::selectedNextPoint(const arma::mat &p)
{
    vtkNew<vtkPointSource> point;
    point->SetCenter(p[0], p[1], p[2]);
    point->SetNumberOfPoints(1);
    point->SetRadius(0);
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( vtkSmartPointer<vtkPolyDataMapper>::New() );
    actor->GetMapper()->SetInputConnection(point->GetOutputPort());
    auto prop=actor->GetProperty();
    prop->SetRepresentationToPoints();
    prop->SetPointSize(8);
    prop->SetColor(1., 0, 0);

    viewer().renderer()->AddActor(actor);

    if (os_.pCtr_.empty())
    {
        os_.pCtr_=p;
        pAct_[0]=actor;
        userPrompt("Please select second point!");
    }
    else if (os_.p2_.empty())
    {
        os_.p2_=p;
        pAct_[1]=actor;

        OrientBackgroundImageCoordinatesDialog dlg(os_.pCtr_, os_.p2_, &viewer());

        if (dlg.exec()==QDialog::Accepted)
        {
            os_.xy1_=dlg.p1();
            os_.xy2_=dlg.p2();
            orientationSelected(os_);
        }

        actionIsFinished(true);
    }
}

IQVTKOrientBackgroundImage::IQVTKOrientBackgroundImage(IQVTKCADModel3DViewer &viewWidget, vtkImageActor* imageActor)
  : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget),
    imageActor_(imageActor)
{}



IQVTKOrientBackgroundImage::~IQVTKOrientBackgroundImage()
{
    for (auto pAct: pAct_)
    {
        viewer().renderer()->RemoveActor(pAct);
    }
}

void IQVTKOrientBackgroundImage::start()
{
    userPrompt("Please pick the origin!");
}


bool IQVTKOrientBackgroundImage::onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    auto picker = vtkSmartPointer<vtkPropPicker>::New();
    auto p = viewer().widgetCoordsToVTK(point);
    picker->Pick(p.x(), p.y(), 0, viewer().renderer());

    // There could be other props assigned to this picker, so
    // make sure we picked the image actor.
    vtkAssemblyPath* path = picker->GetPath();
    bool validPick = false;

    viewer().scheduleRedraw();

    if (path)
    {
        vtkCollectionSimpleIterator sit;
        path->InitTraversal(sit);
        for (int i = 0; i < path->GetNumberOfItems() && !validPick; ++i)
        {
            auto node = path->GetNextNode(sit);
            if (imageActor_ == dynamic_cast<vtkImageActor*>(node->GetViewProp()))
            {
                validPick = true;
            }
        }
    }

    if (!validPick)
    {
        userPrompt("Please pick on image!");
    }
    else
    {
        // Get the world coordinates of the pick.
        arma::mat p=insight::vec3Zero();
        picker->GetPickPosition(p.memptr());
        selectedNextPoint(p);
        return true;
    }

    return ViewWidgetAction<IQVTKCADModel3DViewer>::onLeftButtonDown(nFlags, point);
}

