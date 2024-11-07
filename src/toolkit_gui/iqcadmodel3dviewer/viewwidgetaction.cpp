#include "viewwidgetaction.h"
#include "qoccviewwidget.h"

#include "TopoDS.hxx"
#include "TopoDS_Shape.hxx"
#include "BRep_Tool.hxx"

#include "occtools.h"
#include "occguitools.h"
#include "cadpostprocactions.h"





ToNotepadEmitter::ToNotepadEmitter()
    : QObject(nullptr)
{}

ToNotepadEmitter::~ToNotepadEmitter()
{}





OCCViewWidgetRotation::OCCViewWidgetRotation(QoccViewWidget &viewWidget, const QPoint point)
  : ViewWidgetAction<QoccViewWidget>(viewWidget)
{
    viewer().view().StartRotation(point.x(), point.y());
}

void OCCViewWidgetRotation::start()
{}

bool OCCViewWidgetRotation::onMouseMove
  (
   Qt::MouseButtons btn,
   const QPoint point,
   Qt::KeyboardModifiers mods
   )
{
    viewer().view().Rotation( point.x(), point.y() );
    return ViewWidgetAction<QoccViewWidget>::onMouseMove(btn, point, mods);
}




OCCViewWidgetPanning::OCCViewWidgetPanning(QoccViewWidget &viewWidget, const QPoint point)
    : ViewWidgetAction<QoccViewWidget>(viewWidget, point)
{}

void OCCViewWidgetPanning::start()
{}


bool OCCViewWidgetPanning::onMouseMove
  (
   Qt::MouseButtons btn,
   const QPoint point,
   Qt::KeyboardModifiers mods
   )
{
  if (hasLastMouseLocation())
  {
    viewer().view().Pan( point.x() - lastMouseLocation().x(),
               lastMouseLocation().y() - point.y() );
  }
  return ViewWidgetAction<QoccViewWidget>::onMouseMove(btn, point, mods);
}




OCCViewWidgetDynamicZooming::OCCViewWidgetDynamicZooming(QoccViewWidget &viewWidget, const QPoint point)
    : ViewWidgetAction<QoccViewWidget>(viewWidget, point)
{}

void OCCViewWidgetDynamicZooming::start()
{}


bool OCCViewWidgetDynamicZooming::onMouseMove
  (
   Qt::MouseButtons btn,
   const QPoint point,
   Qt::KeyboardModifiers mods
   )
{
    if (hasLastMouseLocation())
    {
        viewer().view().Zoom( lastMouseLocation().x(), lastMouseLocation().y(),
                              point.x(), point.y() );
    }

    return ViewWidgetAction<QoccViewWidget>::onMouseMove(btn, point, mods);
}




OCCViewWidgetWindowZooming::OCCViewWidgetWindowZooming(QoccViewWidget &viewWidget, const QPoint point, QRubberBand *rb)
  : ViewWidgetAction<QoccViewWidget>(viewWidget, point), rb_(rb)
{}

OCCViewWidgetWindowZooming::~OCCViewWidgetWindowZooming()
{
  auto r=rb_->rect();
  viewer().view().WindowFitAll(
        r.topLeft().x(),
        r.topLeft().y(),
        r.bottomRight().x(),
      r.bottomRight().y() );
}

void OCCViewWidgetWindowZooming::start()
{}


bool OCCViewWidgetWindowZooming::onMouseMove
  (
   Qt::MouseButtons btn,
   const QPoint point,
   Qt::KeyboardModifiers mods
   )
{
    if (hasLastMouseLocation())
    {
      rb_->hide();
      rb_->setGeometry ( QRect(lastMouseLocation(), point).normalized() );
      rb_->show();

      viewer().view().Zoom( lastMouseLocation().x(), lastMouseLocation().y(),
                   point.x(), point.y() );
    }

    return ViewWidgetAction<QoccViewWidget>::onMouseMove(btn, point, mods);
}



OCCViewWidgetMeasurePoints::OCCViewWidgetMeasurePoints(QoccViewWidget &viewWidget)
  : ViewWidgetAction<QoccViewWidget>(viewWidget)
{}

OCCViewWidgetMeasurePoints::~OCCViewWidgetMeasurePoints()
{
  insight::cad::DeactivateAll(viewer().getContext(), TopAbs_VERTEX);
}

void OCCViewWidgetMeasurePoints::start()
{
  insight::cad::ActivateAll(viewer().getContext(), TopAbs_VERTEX);
  userPrompt("Please select first point!");
}

bool OCCViewWidgetMeasurePoints::onLeftButtonUp(Qt::KeyboardModifiers /*nFlags*/, const QPoint /*point*/,
                                                bool lastClickWasDoubleClick)
{
  viewer().getContext()->InitSelected();
  if (viewer().getContext()->MoreSelected())
  {
    TopoDS_Shape v = viewer().getContext()->SelectedShape();

    gp_Pnt p =BRep_Tool::Pnt(TopoDS::Vertex(v));
    std::cout<< p.X() <<" "<<p.Y()<< " " << p.Z()<<std::endl;

    if (!p1_)
      {
        p1_=insight::cad::matconst(insight::vec3(p));
        userPrompt("Please select second point!");
        return true;
      }
    else if (!p2_)
      {
        p2_=insight::cad::matconst(insight::vec3(p));
        userPrompt("Measurement is created...");

        viewer().addEvaluationToModel
            (
              "distance measurement",
              insight::cad::PostprocActionPtr
              (
                new insight::cad::Distance(p1_, p2_)
              ),
              true
            );
        
        finishAction();
        return true;
      }
  }
  return false;
}

