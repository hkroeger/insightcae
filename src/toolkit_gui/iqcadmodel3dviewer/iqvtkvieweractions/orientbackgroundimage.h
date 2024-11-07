#ifndef INSIGHT_ORIENTBACKGROUNDIMAGE_H
#define INSIGHT_ORIENTBACKGROUNDIMAGE_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"

#include "vtkSmartPointer.h"
#include "vtkActor.h"

class IQVTKCADModel3DViewer;
class vtkImageActor;


struct OrientationSpec
{
    arma::mat pCtr_, p2_;
    arma::mat xy1_, xy2_;
};

class IQVTKOrientBackgroundImage
: public ViewWidgetAction<IQVTKCADModel3DViewer>
{
public:
    boost::signals2::signal<void(OrientationSpec)> orientationSelected;

private:
    vtkImageActor* imageActor_;

    vtkSmartPointer<vtkActor> pAct_[2];

    OrientationSpec os_;

    void selectedNextPoint(const arma::mat& p);

public:
    IQVTKOrientBackgroundImage(IQVTKCADModel3DViewer &viewWidget, vtkImageActor* imageActor);
    ~IQVTKOrientBackgroundImage();

    void start() override;

    bool onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point,
                        bool lastClickWasDoubleClick ) override;
};


#endif // INSIGHT_ORIENTBACKGROUNDIMAGE_H
