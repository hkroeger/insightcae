#ifndef IQVTKVIEWER_H
#define IQVTKVIEWER_H


#include "base/linearalgebra.h"
#include "toolkit_gui_export.h"
#include "vtkVersionMacros.h"
#include "vtkGenericOpenGLRenderWindow.h"

#if VTK_MAJOR_VERSION>=8
#include <QVTKOpenGLNativeWidget.h>
#else
#include <QVTKWidget.h>
#endif

typedef
#if VTK_MAJOR_VERSION>=8
    QVTKOpenGLNativeWidget
#else
    QVTKWidget
#endif
        VTKWidget;

#include <QMainWindow>

class vtkImageActor;
class vtkImageData;
class vtkCamera;


class TOOLKIT_GUI_EXPORT IQVTKViewer
: public QMainWindow
{

private:
    VTKWidget vtkWidget_;
    vtkSmartPointer<vtkRenderer> ren_;

public:
    IQVTKViewer(QWidget* parent=nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    QPointF widgetCoordsToVTK(const QPoint &widgetCoords) const;

    arma::mat pointInPlane3D(
        const arma::mat& planeNormal, const arma::mat& planeBasePoint,
        const QPoint &screenPos ) const;

    vtkRenderWindowInteractor* interactor();
    vtkRenderer* renderer();
    vtkRenderWindow* renWin();
};




class TOOLKIT_GUI_EXPORT IQImageReferencePointSelectorWindow
: public IQVTKViewer
{
public:
    vtkSmartPointer<vtkImageActor> actor_;

    double origin[3];
    double spacing[3];
    int extent[6];

    std::shared_ptr<arma::mat> p1_, p2_, p3_;
    double L_;

public:
    IQImageReferencePointSelectorWindow(vtkImageActor* toBeDisplayed, QWidget* parent=nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    static void setupCameraForImage(vtkImageData* imd, vtkCamera* cam);

protected:
    void mousePressEvent(QMouseEvent* e) override;
};




#endif // IQVTKVIEWER_H
