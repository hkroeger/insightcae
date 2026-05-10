#ifndef IQVTKMANIPULATECOORDINATESYSTEM_H
#define IQVTKMANIPULATECOORDINATESYSTEM_H

#include "base/linearalgebra.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"

#include <memory>
#include <vtkPointSource.h>
#include <vtkTransformFilter.h>

class IQVTKCADModel3DViewer;
class IQVTKViewerState;


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

    std::shared_ptr<IQVTKViewerState> currentHighlight_;

public:
    enum Actor {
       P0, Ex, Ey, Ez, XY, XZ, YZ
    };

    typedef std::set<Actor> ActorMask;

private:
    ActorMask actorsToShow_;
    bool locPosition_;

    std::map<Actor,vtkSmartPointer<vtkActor> > actors_;

    vtkActor *actorIfActive(Actor a) const;

    struct Manip
    {
        IQVTKManipulateCoordinateSystem* parent;
        virtual void update(const QPoint& newp) =0;
    };

    struct ManipPlane : Manip
    {
        std::function<arma::mat(void)> normal;
        boost::optional<arma::mat> startPt;
        void update(const QPoint& newp) override;
    };

    struct ManipAlong : Manip
    {
        arma::mat p2, d2;

        boost::optional<arma::mat> startDiff_;
        arma::mat closestPt(const QPoint& newp);

        void update(const QPoint& newp) override;
    };

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
        bool locPosition = false,
        ActorMask actorsToShow = {} );

    QString description() const override;

    void start() override;

    bool onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags
        ) override;

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
