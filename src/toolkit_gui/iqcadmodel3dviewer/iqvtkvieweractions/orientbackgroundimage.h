#ifndef INSIGHT_ORIENTBACKGROUNDIMAGE_H
#define INSIGHT_ORIENTBACKGROUNDIMAGE_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqcadmodel3dviewer/iqvtkbackgroundimage.h"

#include "vtkSmartPointer.h"
#include "vtkActor.h"

class IQVTKCADModel3DViewer;
class vtkImageActor;


class IQVTKOrientBackgroundImage
: public ViewWidgetAction<IQVTKCADModel3DViewer>
{
public:
    boost::signals2::signal<void(OrientationSpec)> orientationSelected;

private:
    BackgroundImage& image_;

    vtkSmartPointer<vtkActor> pAct_[2];
    int curPoint_;

    OrientationSpec os_;

    void selectedNextPoint(const arma::mat& p);
    void skip();
    void finalDialog();

public:
    IQVTKOrientBackgroundImage(IQVTKCADModel3DViewer &viewWidget, BackgroundImage& image);

    QString description() const override;

    void start() override;

    bool onKeyPress(
        Qt::KeyboardModifiers modifiers,
        int key ) override;

    bool onMouseClick(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;
};


#endif // INSIGHT_ORIENTBACKGROUNDIMAGE_H
