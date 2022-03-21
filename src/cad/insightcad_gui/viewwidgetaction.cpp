#include "viewwidgetaction.h"
#include "qoccviewwidget.h"

#include "TopoDS.hxx"
#include "TopoDS_Shape.hxx"
#include "BRep_Tool.hxx"

#include "occtools.h"
#include "occguitools.h"
#include "cadpostprocactions.h"



const QPoint &InputReceiver::lastMouseLocation() const
{
  return lastMouseLocation_;
}

void InputReceiver::onLeftButtonDown  ( Qt::KeyboardModifiers, const QPoint )
{}

void InputReceiver::onMiddleButtonDown( Qt::KeyboardModifiers, const QPoint )
{}

void InputReceiver::onRightButtonDown ( Qt::KeyboardModifiers, const QPoint )
{}

void InputReceiver::onLeftButtonUp    ( Qt::KeyboardModifiers, const QPoint )
{}

void InputReceiver::onMiddleButtonUp  ( Qt::KeyboardModifiers, const QPoint )
{}

void InputReceiver::onRightButtonUp   ( Qt::KeyboardModifiers, const QPoint )
{}

void InputReceiver::onKeyPress ( Qt::KeyboardModifiers, int )
{}

void InputReceiver::onKeyRelease ( Qt::KeyboardModifiers, int )
{}

void InputReceiver::onMouseMove
  (
   Qt::MouseButtons,
   Qt::KeyboardModifiers,
   const QPoint point,
   Qt::KeyboardModifiers
   )
{
  lastMouseLocation_=point;
}


void InputReceiver::onMouseWheel(double, double)
{
}






ViewWidgetAction::ViewWidgetAction()
{}

ViewWidgetAction::~ViewWidgetAction()
{}

void ViewWidgetAction::setFinished()
{
  finished_=true;
}





ViewWidgetRotation::ViewWidgetRotation(Handle_V3d_View view, const QPoint point)
  : view_(view)
{
  view_->StartRotation(point.x(), point.y());
}

void ViewWidgetRotation::onMouseMove
  (
   Qt::MouseButtons,
   Qt::KeyboardModifiers,
   const QPoint point,
   Qt::KeyboardModifiers
   )
{
  view_->Rotation( point.x(), point.y() );
}




ViewWidgetPanning::ViewWidgetPanning(Handle_V3d_View view, const QPoint point)
  : view_(view)
{
  startPoint_=point;
}


void ViewWidgetPanning::onMouseMove
  (
   Qt::MouseButtons,
   Qt::KeyboardModifiers,
   const QPoint point,
   Qt::KeyboardModifiers
   )
{
  view_->Pan( point.x() - startPoint_.x(),
               startPoint_.y() - point.y() );
  startPoint_ = point;
}




ViewWidgetDynamicZooming::ViewWidgetDynamicZooming(Handle_V3d_View view, const QPoint point)
  : view_(view)
{
  startPoint_=point;
}


void ViewWidgetDynamicZooming::onMouseMove
  (
   Qt::MouseButtons,
   Qt::KeyboardModifiers,
   const QPoint point,
   Qt::KeyboardModifiers
   )
{
  view_->Zoom( startPoint_.x(), startPoint_.y(),
               point.x(), point.y() );
  startPoint_ = point;
}




ViewWidgetWindowZooming::ViewWidgetWindowZooming(Handle_V3d_View view, const QPoint point, QRubberBand *rb)
  : view_(view), rb_(rb)
{
  startPoint_=point;
}

ViewWidgetWindowZooming::~ViewWidgetWindowZooming()
{
  auto r=rb_->rect();
  view_->WindowFitAll(
        r.topLeft().x(),
        r.topLeft().y(),
        r.bottomRight().x(),
        r.bottomRight().y() );
}


void ViewWidgetWindowZooming::onMouseMove
  (
   Qt::MouseButtons,
   Qt::KeyboardModifiers,
   const QPoint point,
   Qt::KeyboardModifiers
   )
{
  rb_->hide();
  rb_->setGeometry ( QRect(startPoint_, point).normalized() );
  rb_->show();

  view_->Zoom( startPoint_.x(), startPoint_.y(),
               point.x(), point.y() );
  startPoint_ = point;
}




ViewWidgetMeasurePoints::ViewWidgetMeasurePoints(QoccViewWidget *viewWidget)
  : viewWidget_(viewWidget)
{
  insight::cad::ActivateAll(viewWidget_->getContext(), TopAbs_VERTEX);
  viewWidget_->sendStatus("Please select first point!");
}

ViewWidgetMeasurePoints::~ViewWidgetMeasurePoints()
{
  insight::cad::DeactivateAll(viewWidget_->getContext(), TopAbs_VERTEX);
}

void ViewWidgetMeasurePoints::onLeftButtonUp(Qt::KeyboardModifiers /*nFlags*/, const QPoint /*point*/)
{
  viewWidget_->getContext()->InitSelected();
  if (viewWidget_->getContext()->MoreSelected())
  {
    TopoDS_Shape v = viewWidget_->getContext()->SelectedShape();

    gp_Pnt p =BRep_Tool::Pnt(TopoDS::Vertex(v));
    std::cout<< p.X() <<" "<<p.Y()<< " " << p.Z()<<std::endl;

    if (!p1_)
      {
        p1_=insight::cad::matconst(insight::vec3(p));
        viewWidget_->sendStatus("Please select second point!");
      }
    else if (!p2_)
      {
        p2_=insight::cad::matconst(insight::vec3(p));
        viewWidget_->sendStatus("Measurement is created...");

        viewWidget_->addEvaluationToModel
            (
              "distance measurement",
              insight::cad::PostprocActionPtr
              (
                new insight::cad::Distance(p1_, p2_)
              ),
              true
            );

        setFinished();
      }
  }
}





