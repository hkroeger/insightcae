#include "iqvtkconstrainedsketcheditor.h"
#include "base/units.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkProperty.h"

#include "iqvtkvieweractions/iqvtkcadmodel3dviewerdrawline.h"
#include "cadpostprocactions/pointdistance.h"
#include "cadpostprocactions/angle.h"

#include <QInputDialog>
#include <QMessageBox>


void IQVTKConstrainedSketchEditor::add(insight::cad::ConstrainedSketchGeometryPtr sg)
{
    double color[3]={1,0,0};
    double lw=2;
    std::vector<vtkSmartPointer<vtkProp> > acs;
    if (auto feat = std::dynamic_pointer_cast<insight::cad::Feature>(sg))
    {
        acs=viewer().createActor(feat);
    }
    else if (auto pt = std::dynamic_pointer_cast<insight::cad::Vector>(sg))
    {
        acs=viewer().createActor(pt);
    }
    else if (auto pa = std::dynamic_pointer_cast<insight::cad::PostprocAction>(sg)) // dimensions etc.
    {
        acs=viewer().createActor(pa);
        color[0]=0.7;
        color[1]=color[2]=0.3;
        lw=0.5;
    }
    ActorSet as;
    for (auto& a: acs)
    {
        if (vtkSmartPointer<vtkActor> aa = vtkActor::SafeDownCast(a))
        {
            aa->GetProperty()->SetColor(color);
            aa->GetProperty()->SetLineWidth(lw);
        }
        viewer().renderer()->AddActor(a);
        as.insert(a);
    }
    sketchGeometryActors_.emplace(sg, as);
}




void IQVTKConstrainedSketchEditor::remove(insight::cad::ConstrainedSketchGeometryPtr sg)
{
    auto i=sketchGeometryActors_.find(sg);
    if (i!=sketchGeometryActors_.end())
    {
       ActorSet actors = i->second;
       sketchGeometryActors_.erase(i);
       for(auto &a: actors)
       {
           viewer().renderer()->RemoveActor(a);
       }
    }
}

void IQVTKConstrainedSketchEditor::drawLine()
{
    auto dl = std::make_shared<IQVTKCADModel3DViewerDrawLine>(viewer(), *this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::updateActors,
            this, &IQVTKConstrainedSketchEditor::updateActors );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::lineAdded, dl.get(),
            [this](insight::cad::Line* line, insight::cad::Line* prevLine)
            {
                double curLen = arma::norm( line->start()->value() - line->end()->value(), 2);

                auto lc = std::make_shared<insight::cad::DistanceConstraint>(
                        line->start(), line->end(),
                            insight::cad::scalarconst(curLen) );
                (*this)->geometry().insert(lc);

                if (prevLine)
                {
                    auto ac = std::make_shared<insight::cad::AngleConstraint>(
                            prevLine->start(), line->end(), line->start(),
                                insight::cad::scalarconst(
                                    insight::cad::Angle::calculate(
                                        prevLine->start()->value(),
                                        line->end()->value(),
                                        line->start()->value() ) ) );
                    (*this)->geometry().insert(ac);
                }


                (*this)->invalidate();
                this->updateActors();
            }
    );

    currentAction_=dl;
}




void IQVTKConstrainedSketchEditor::solve()
{
    (*this)->resolveConstraints();
    this->updateActors();
}




IQVTKConstrainedSketchEditor::IQVTKConstrainedSketchEditor(
        IQVTKCADModel3DViewer& viewer,
        insight::cad::ConstrainedSketchPtr sketch
)
    : ViewWidgetAction<IQVTKCADModel3DViewer>(viewer),
      insight::cad::ConstrainedSketchPtr(sketch)
{
    toolBar_ = this->viewer().addToolBar("Sketcher commands");
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawline.svg"), "Line",
                        this, &IQVTKConstrainedSketchEditor::drawLine);
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_solve.svg"), "Solve",
                        this, &IQVTKConstrainedSketchEditor::solve);
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_finish.svg"), "Finish",
                        this, &IQVTKConstrainedSketchEditor::finished);
    toolBar_->show();

    updateActors();
}




IQVTKConstrainedSketchEditor::~IQVTKConstrainedSketchEditor()
{
    toolBar_->hide();
    toolBar_->deleteLater();

    // copy because "remove" invalidates for-loop
    std::set<insight::cad::ConstrainedSketchGeometryPtr> sgs;

    std::transform(sketchGeometryActors_.begin(), sketchGeometryActors_.end(),
        std::inserter(sgs, sgs.end()),
        [](const SketchGeometryActorMap::value_type& v){ return v.first; } );

    for (const auto& sg: sgs)
    {
        remove(sg);
    }
}




insight::cad::ConstrainedSketchGeometryPtr
IQVTKConstrainedSketchEditor::findSketchElementOfActor
    (vtkActor *actor) const
{
    for (const auto& sga: sketchGeometryActors_)
    {
        for (const auto& a: sga.second)
        {
            if (actor==a)
            {
                return sga.first;
            }
        }
    }
    return nullptr;
}


void IQVTKConstrainedSketchEditor::onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    if (currentAction_)
        currentAction_->onLeftButtonDown( nFlags, point );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

}

void IQVTKConstrainedSketchEditor::onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    if (currentAction_)
        currentAction_->onMiddleButtonDown( nFlags, point );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}

void IQVTKConstrainedSketchEditor::onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    if (currentAction_)
        currentAction_->onRightButtonDown( nFlags, point );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}

void IQVTKConstrainedSketchEditor::onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    if (currentAction_)
        currentAction_->onLeftButtonUp( nFlags, point );
    else
    {
        if (auto act =
                viewer().findActorUnderCursorAt(point))
        {
            if (auto sg =
                    this->findSketchElementOfActor(act))
            {
                if (auto dc =
                        std::dynamic_pointer_cast
                         <insight::cad::DistanceConstraint>(sg))
                {
                    if (auto cs =
                            std::dynamic_pointer_cast<insight::cad::ConstantScalar>(
                                dc->targetValue() ) )
                    {
                        bool ok;
                        double v = QInputDialog::getDouble(
                                    &viewer(),
                                    "Modify distance constraint",
                                    "Enter distance constraint value",
                                    cs->value(),
                                    0, DBL_MAX, 6, &ok);
                        if (ok)
                        {
                            cs->setValue(v);
                        }
                    }
                    else
                    {
                        QMessageBox::critical(
                                    &viewer(),
                                    "Cannot modify constraint",
                                    "The selected constraint cannot be modified!" );
                    }
                }
                else if (auto dc =
                        std::dynamic_pointer_cast
                         <insight::cad::AngleConstraint>(sg))
                {
                    if (auto cs =
                            std::dynamic_pointer_cast<insight::cad::ConstantScalar>(
                                dc->targetValue() ) )
                    {
                        bool ok;
                        double v = QInputDialog::getDouble(
                                    &viewer(),
                                    "Modify angle constraint",
                                    "Enter angle constraint value [deg]",
                                    cs->value()/SI::deg,
                                    0, DBL_MAX, 6, &ok);
                        if (ok)
                        {
                            cs->setValue(v*SI::deg);
                        }
                    }
                    else
                    {
                        QMessageBox::critical(
                                    &viewer(),
                                    "Cannot modify constraint",
                                    "The selected constraint cannot be modified!" );
                    }
                }
            }
        }
    }
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}

void IQVTKConstrainedSketchEditor::onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    if (currentAction_)
        currentAction_->onMiddleButtonUp( nFlags, point );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}

void IQVTKConstrainedSketchEditor::onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    if (currentAction_)
        currentAction_->onRightButtonUp( nFlags, point );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}

void IQVTKConstrainedSketchEditor::onKeyPress ( Qt::KeyboardModifiers modifiers, int key )
{
    if (currentAction_)
        currentAction_->onKeyPress( modifiers, key );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}

void IQVTKConstrainedSketchEditor::onKeyRelease ( Qt::KeyboardModifiers modifiers, int key )
{
    if (currentAction_)
        currentAction_->onKeyRelease( modifiers, key );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}

void IQVTKConstrainedSketchEditor::onMouseMove
  (
   Qt::MouseButtons buttons,
   const QPoint point,
   Qt::KeyboardModifiers curFlags
   )
{
    if (currentAction_)
        currentAction_->onMouseMove( buttons, point, curFlags );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}

void IQVTKConstrainedSketchEditor::onMouseWheel
  (
    double angleDeltaX,
    double angleDeltaY
   )
{
    if (currentAction_)
        currentAction_->onMouseWheel( angleDeltaX, angleDeltaY );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
}


void IQVTKConstrainedSketchEditor::updateActors()
{

  for (const auto& g: (*this)->geometry())
  {
      auto i=sketchGeometryActors_.find(g);
      if (i!=sketchGeometryActors_.end())
      {
          remove(g);
      }
      add(g);
  }

  for (const auto& sga: sketchGeometryActors_)
  {
      auto g=sga.first;
      auto i=this->get()->geometry().find(g);
      if (i==this->get()->geometry().end())
      {
        remove(g);
      }
  }
}
