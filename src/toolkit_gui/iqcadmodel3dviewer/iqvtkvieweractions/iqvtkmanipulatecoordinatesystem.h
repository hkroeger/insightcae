#ifndef IQVTKMANIPULATECOORDINATESYSTEM_H
#define IQVTKMANIPULATECOORDINATESYSTEM_H

#include "base/linearalgebra.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"

#include <memory>
#include <vtkPointSource.h>
#include <vtkTransformFilter.h>

class IQVTKCADModel3DViewer;

class IQVTKManipulateCoordinateSystem
    : public QObject,
      public ViewWidgetAction<IQVTKCADModel3DViewer>
{
    Q_OBJECT

    bool showOnlyX_;
    insight::CoordinateSystem cs_;

    vtkSmartPointer<vtkPointSource>
        ctr_;

    vtkSmartPointer<vtkTransformFilter>
        ex_, ey_, ez_, pxy_, pxz_, pyz_;

    vtkSmartPointer<vtkActor>
        aP0_, aEx_, aEy_, aEz_, aXY_, aXZ_, aYZ_;

    struct Manip
    {
        IQVTKManipulateCoordinateSystem* parent;
        boost::optional<QPoint> start;
        virtual void update(const QPoint& newp) =0;
    };

    struct ManipPlane : Manip
    {
        std::function<arma::mat(void)> normal;
        void update(const QPoint& newp) override;
    };
    // struct ManipEX : Manip
    // {
    //     void update(const QPoint& newp) override;
    // };
    // struct ManipEY : Manip
    // {
    //     void update(const QPoint& newp) override;
    // };
    // struct ManipEZ : Manip
    // {
    //     void update(const QPoint& newp) override;
    // };
    struct ManipP0 : Manip
    {
        void update(const QPoint& newp) override;
    };

    std::unique_ptr<Manip> action;

    void setCS();

public:
    IQVTKManipulateCoordinateSystem(
        ViewWidgetActionHost<IQVTKCADModel3DViewer> &parent,
        const insight::CoordinateSystem& cs,
        bool showOnlyX=false );

    QString description() const override;

    void start() override;

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType ) override;

    bool onMouseClick(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

    bool onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override;

Q_SIGNALS:
    void coordinateSystemSelected(const insight::CoordinateSystem&);
};

#endif // IQVTKMANIPULATECOORDINATESYSTEM_H
