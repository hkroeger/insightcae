#include "orientbackgroundimage.h"
#include "base/exception.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkImageActor.h"
#include "vtkPointSource.h"

#include "orientbackgroundimagecoordinatesdialog.h"
#include <qnamespace.h>





vtkSmartPointer<vtkActor> createPointActor(const arma::mat& p)
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

    return actor;
}




void IQVTKOrientBackgroundImage::selectedNextPoint(const arma::mat &p)
{
    if (curPoint_>=0 && curPoint_<2)
    {
        auto oldTrsf=image_.orientation().trsf().inverted();

        auto actor=createPointActor(p);

        viewer().renderer()->RemoveActor(pAct_[curPoint_]);
        pAct_[curPoint_]=actor;
        viewer().renderer()->AddActor(actor);
        viewer().scheduleRedraw();
        os_.p_[curPoint_]=oldTrsf.trsfPt(p); // where it were on the original image
    }

    switch (curPoint_)
    {
    case 0: {
        userPrompt("Please select second point!");
        curPoint_++;
    } break;

    case 1: {
        finalDialog();
    } break;

    default:
        throw insight::UnhandledSelection();
    }
}

void IQVTKOrientBackgroundImage::skip()
{
    os_.p_[curPoint_]=image_.orientation().p_[curPoint_];

    if (curPoint_<1)
    {
        userPrompt("Keeping first point. Please select second point!");
        curPoint_++;
    }
    else
    {
        finalDialog();
    }
}


void IQVTKOrientBackgroundImage::finalDialog()
{
    auto oldTrsf=image_.orientation().trsf();

    OrientBackgroundImageCoordinatesDialog dlg(
        oldTrsf.trsfPt(os_.p_[0]),
        oldTrsf.trsfPt(os_.p_[1]),
        &viewer());

    if (dlg.exec()==QDialog::Accepted)
    {
        os_.xy_[0]=dlg.xy1();
        os_.xy_[1]=dlg.xy2();
        orientationSelected(os_);
    }

    finishAction(true);
}



IQVTKOrientBackgroundImage::IQVTKOrientBackgroundImage(
    IQVTKCADModel3DViewer &viewWidget,
    BackgroundImage& image
    )
  : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget, false),
    image_(image), curPoint_(0)
{
    auto os=image_.orientation();
    for (int i=0; i<2; ++i)
    {
        pAct_[i]=createPointActor(os.xy_[i]);
        viewer().renderer()->AddActor(pAct_[i]);
    }
    viewer().scheduleRedraw();

    aboutToBeDestroyed.connect(
        [this](){
            DBG_SLOT(aboutToBeDestroyed);

            for (auto pAct: pAct_)
            {
                viewer().renderer()->RemoveActor(pAct);
            }
            viewer().scheduleRedraw();
        });
}




QString IQVTKOrientBackgroundImage::description() const
{
    return "Orient background image";
}



void IQVTKOrientBackgroundImage::start()
{
    userPrompt("Please pick the origin!");
}


bool IQVTKOrientBackgroundImage::onKeyPress(
    Qt::KeyboardModifiers modifiers,
    int key )
{
    if (key == Qt::Key_Enter)
    {
        skip();
        return true;
    }

    return ViewWidgetAction<IQVTKCADModel3DViewer>
        ::onKeyPress( modifiers, key );
}


bool IQVTKOrientBackgroundImage::onMouseClick(
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    if (btn==Qt::LeftButton)
    {
        auto picker = vtkSmartPointer<vtkPropPicker>::New();
        picker->AddPickList(image_.imageActor());
        picker->SetPickFromList(true);

        auto p = viewer().widgetCoordsToVTK(point);
        int np = picker->PickProp(p.x(), p.y(), viewer().renderer());

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
                if (image_.imageActor() == dynamic_cast<vtkImageActor*>(node->GetViewProp()))
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
    }
    else if (btn==Qt::MiddleButton)
    {
        skip();
        return true;
    }
    else if (btn==Qt::RightButton)
    {
        userPrompt("Image orientation procedure cancelled.");
        finishAction(false);
        return true;
    }

    return ViewWidgetAction<IQVTKCADModel3DViewer>::onMouseClick(
        btn, nFlags, point );
}

