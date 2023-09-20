#include "iqvtkconstrainedsketcheditor.h"
#include "base/units.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkLineRepresentation.h"
#include "vtkProperty.h"
#include "vtkSphereSource.h"
#include "vtkVectorText.h"
#include "vtkFollower.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkCaptionActor2D.h"

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewerdrawline.h"
#include "cadpostprocactions/pointdistance.h"
#include "cadpostprocactions/angle.h"
#include "base/parameters/simpleparameter.h"



#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>

#include "iqvtkconstrainedsketcheditor/iqvtkfixedpoint.h"
#include "iqvtkconstrainedsketcheditor/iqvtkhorizontalconstraint.h"
#include "iqvtkconstrainedsketcheditor/iqvtkverticalconstraint.h"
#include "iqvtkconstrainedsketcheditor/iqvtkpointoncurveconstraint.h"

#include "base/qt5_helper.h"
#include "datum.h"






void IQVTKConstrainedSketchEditor::add(
    insight::cad::ConstrainedSketchEntityPtr sg)
{
    auto setapp = setActorAppearance_;
    std::vector<vtkSmartPointer<vtkProp> > acs;

    if (auto feat = std::dynamic_pointer_cast<
            insight::cad::Feature>(sg))
    {
        acs=viewer().createActor(feat);
    }
    else if (auto pt = std::dynamic_pointer_cast<
                   insight::cad::Vector>(sg))
    {
        acs=viewer().createActor(pt);
    }
    else if (auto pa = std::dynamic_pointer_cast<
                   insight::cad::PostprocAction>(sg))
    {
        // dimensions etc.
        acs=viewer().createActor(pa);
        setapp=[](const insight::ParameterSet& p, vtkProperty* prop)
            {
                prop->SetColor(0.7, 0.3, 0.3);
                prop->SetLineWidth(0.5);
            };
    }
    else if (auto soe = std::dynamic_pointer_cast<
                   IQVTKConstrainedSketchEntity>(sg))
    {
        // e.g. constrains etc.
        acs=soe->createActor();
    }

    ActorSet as;
    for (auto& a: acs)
    {
        if (vtkSmartPointer<vtkActor> aa = vtkActor::SafeDownCast(a))
        {
            setapp(sg->parameters(), aa->GetProperty());
        }
        if (auto follower=vtkFollower::SafeDownCast(a))
        {
            follower->SetCamera(
                viewer().renderer()->GetActiveCamera());
        }
        viewer().renderer()->AddActor(a);
        as.insert(a);
    }

    sketchGeometryActors_.emplace(sg, as);
}




void IQVTKConstrainedSketchEditor::remove(
    insight::cad::ConstrainedSketchEntityPtr sg)
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



bool IQVTKConstrainedSketchEditor::defaultSelectionActionRunning()
{
    return isRunning<IQVTKSelectConstrainedSketchEntity>();
}



void IQVTKConstrainedSketchEditor::launchDefaultSelectionAction()
{
    auto sel = std::make_shared<IQVTKSelectConstrainedSketchEntity>(*this);
    launchChildAction(sel);
}







void IQVTKConstrainedSketchEditor::drawLine()
{
    auto dl = std::make_shared<IQVTKCADModel3DViewerDrawLine>(*this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::updateActors, dl.get(),
            [this]() { updateActors(); } );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::endPointSelected, dl.get(),

            [this]( IQVTKCADModel3DViewerDrawLine::EndPointProperty* addPoint,
                    insight::cad::SketchPointPtr previousPoint )
            {
                if (addPoint->onFeature)
                {
                    (*this)->geometry().insert(
                        IQVTKPointOnCurveConstraint::create(
                            addPoint->p,
                            addPoint->onFeature ) ); // fix to curve

                    if (!previousPoint)
                    {
                        // for first point on line: add distance constraint to beginning of line
                        if (auto online =
                            std::dynamic_pointer_cast<insight::cad::Line>(
                                addPoint->onFeature ) )
                        {
                            double curLen = arma::norm(
                                online->start()->value()
                                    - addPoint->p->value(),
                                2 );

                            auto lc = insight::cad::DistanceConstraint::create(
                                online->start(), addPoint->p,
                                curLen );
                            (*this)->geometry().insert(lc);
                        }
                    }
                }
                else if ( !addPoint->isAnExistingPoint && !previousPoint ) // is first point?
                {
                    (*this)->geometry().insert(
                        IQVTKFixedPoint::create( addPoint->p ) ); // fix first point
                }
            }
    );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::lineAdded, dl.get(),

            [this]( insight::cad::Line* line,
                    insight::cad::Line* prevLine,
                    IQVTKCADModel3DViewerDrawLine::EndPointProperty* p2,
                    IQVTKCADModel3DViewerDrawLine::EndPointProperty* p1 )
            {
                line->changeDefaultParameters(defaultGeometryParameters_);

                if (!p2->isAnExistingPoint)
                {
                    double curLen = arma::norm(
                        line->start()->value()
                            - line->end()->value(),
                        2 );

                    auto lc = insight::cad::DistanceConstraint::create(
                            line->start(), line->end(),
                                curLen );
                    (*this)->geometry().insert(lc);

                    if (!p1->isAnExistingPoint && !p2->isAnExistingPoint
                        && !p1->onFeature && !p2->onFeature)
                    {
                        // add angle constraint
                        if (prevLine)
                        {
                            auto ac = insight::cad::AngleConstraint::create(
                                    prevLine->start(), line->end(), line->start(),
                                        insight::cad::Angle::calculate(
                                                prevLine->start()->value(),
                                                line->end()->value(),
                                                line->start()->value() ) );
                            (*this)->geometry().insert(ac);
                        }
                        else
                        {
                            auto p1=std::make_shared<insight::cad::AddedVector>(
                                        line->start(),
                                        insight::cad::vec3const(1,0,0) );
                            auto ac = insight::cad::AngleConstraint::create(
                                p1, line->end(), line->start(),
                                insight::cad::Angle::calculate(
                                    p1->value(),
                                    line->end()->value(),
                                    line->start()->value() ) );
                            (*this)->geometry().insert(ac);
                        }
                    }


                    (*this)->invalidate();
                    this->updateActors();
                }
            }
    );

    dl->actionIsFinished.connect(
        std::bind(&IQVTKConstrainedSketchEditor::launchDefaultSelectionAction, this) );
    launchChildAction(dl);
}




void IQVTKConstrainedSketchEditor::solve()
{
    (*this)->resolveConstraints(
        [&]()
        {
            this->updateActors(0);
        }
        );
    this->updateActors();
}




IQVTKConstrainedSketchEditor::IQVTKConstrainedSketchEditor(
        IQVTKCADModel3DViewer& viewer,
        insight::cad::ConstrainedSketchPtr sketch,
        const insight::ParameterSet& defaultGeometryParameters,
        IQCADModel3DViewer::SetSketchEntityAppearanceCallback sseac
)
    : ViewWidgetAction<IQVTKCADModel3DViewer>(viewer),
//      IQVTKConstrainedSketchEditorSelectionLogic(
//        [this]()
//        { return std::make_shared<SketchEntityMultiSelection>(*this); },
//        viewer ),
      insight::cad::ConstrainedSketchPtr(sketch),
      setActorAppearance_(sseac),
      defaultGeometryParameters_(defaultGeometryParameters)

{
    launchDefaultSelectionAction();

    toolBar_ = this->viewer().addToolBar("Sketcher commands");
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawline.svg"), "Line",
                        this, &IQVTKConstrainedSketchEditor::drawLine);
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_solve.svg"), "Solve",
                        this, &IQVTKConstrainedSketchEditor::solve);
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_finish.svg"), "Finish",
                        this, &IQVTKConstrainedSketchEditor::finished);

    toolBar_->addAction(
        "H",
        [&]()
        {
            if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
            {
                if (selact->somethingSelected())
                {
                    for (auto &sele: selact->currentSelection())
                    {
                        auto sg = sele.lock();
                        if (auto l = std::dynamic_pointer_cast<insight::cad::Line>(sg))
                        {
                            auto lc = IQVTKHorizontalConstraint::create( l );
                            (*this)->geometry().insert(lc);
                            (*this)->invalidate();
                            this->updateActors();
                        }
                    }
                }
            }
        }
    );
    toolBar_->addAction(
        "V",
        [&]()
        {
            if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
            {
                if (selact->somethingSelected())
                {
                    for (auto &sele: selact->currentSelection())
                    {
                        auto sg = sele.lock();
                        if (auto l = std::dynamic_pointer_cast<insight::cad::Line>(sg))
                        {
                            auto lc = IQVTKVerticalConstraint::create( l );
                            (*this)->geometry().insert(lc);
                            (*this)->invalidate();
                            this->updateActors();
                        }
                    }
                }
            }
        }
    );

    toolBar_->addAction(
        "X",
        [&]()
        {
            if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
            {
                if (selact->somethingSelected())
                {
                    insight::cad::SketchPointPtr pt;

                    for (auto &sele: selact->currentSelection())
                    {
                        insight::assertion(
                            selact->currentSelection().size()==1,
                            "exactly one entity should be selected!");

                        auto sg = sele.lock();
                        if (auto p = std::dynamic_pointer_cast<insight::cad::SketchPoint>(sg))
                        {
                            auto c = IQVTKFixedPoint::create( p );
                            (*this)->geometry().insert(c);
                            (*this)->invalidate();
                            this->updateActors();
                        }
                    }
                }
            }
        }
    );

    toolBar_->addAction(
        QPixmap(":/icons/icon_pointoncurve.svg"), "Point on curve",
        [&]()
        {
            if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
            {
                if (selact->somethingSelected())
                {
                    insight::cad::FeaturePtr curve;
                    insight::cad::SketchPointPtr pt;

                    for (auto &sele: selact->currentSelection())
                    {
                        auto sg = sele.lock();
                        if (auto l = std::dynamic_pointer_cast<insight::cad::Line>(sg))
                        {
                            curve=l;
                        }
                        else if (auto p = std::dynamic_pointer_cast<insight::cad::SketchPoint>(sg))
                        {
                            pt=p;
                        }

                        if (pt && curve)
                        {
                            auto lc = IQVTKPointOnCurveConstraint::create( pt, curve );
                            (*this)->geometry().insert(lc);
                            (*this)->invalidate();
                            this->updateActors();
                        }
                    }
                }
            }
        }
    );

    toolBar_->addAction(
        QPixmap(":/icons/icon_distance.svg"), "Distance",
        [&]()
        {
            if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
            {
                if (selact->somethingSelected())
                {
                    insight::cad::SketchPointPtr p1, p2;

                    if (selact->currentSelection().size()==2)
                    {
                        auto si=selact->currentSelection().begin();
                        p1 = std::dynamic_pointer_cast<insight::cad::SketchPoint>(    si ->lock() );
                        p2 = std::dynamic_pointer_cast<insight::cad::SketchPoint>( (++si)->lock() );
                    }

                    insight::assertion(
                        bool(p1)&&bool(p2),
                        "Please select exactly two sketch points!");

                    auto dc = insight::cad::DistanceConstraint::create( p1, p2 );
                    (*this)->geometry().insert(dc);
                    (*this)->invalidate();
                    this->updateActors();
                }
            }
        }
        );


    toolBar_->addAction(
        QPixmap(":/icons/icon_mergepoints.svg"), "Merge points",
        [&]()
        {
            if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
            {
                if (selact->somethingSelected())
                {
                    insight::assertion(
                        selact->currentSelection().size()==2,
                        "Exactly two points have to be selected!");

                    auto p1 = std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                                    selact->currentSelection().begin()
                                        ->lock());
                    insight::assertion(
                        bool(p1), "one entity is not a point!");

                    auto p2 = std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                        (++selact->currentSelection().begin())
                            ->lock());
                    insight::assertion(
                        bool(p2), "one entity is not a point!");

                    for (auto& e: (*this)->geometry())
                    {
                        e->replaceDependency(p1, p2);
                    }
                    (*this)->geometry().erase(p1);
                    (*this)->invalidate();
                    this->updateActors();
                }
            }
        }
    );

    toolBar_->addAction(
        "Scale",
        [&]()
        {
            bool ok;
            double sf=QInputDialog::getDouble(
                this,
                "Scale Sketch",
                "Enter scale factor:", 1.,
                -DBL_MAX, DBL_MAX, -1, &ok);
            if (ok)
            {
                for (auto& geom: (*this)->geometry())
                {
                    geom->scaleSketch(sf);
                }
                (*this)->invalidate();
                this->updateActors();
            }
        }
    );
    toolBar_->show();


    updateActors();

    // this editor is its own property widget

    toolBoxWidget_ = new QDockWidget("Properties", this);
    this->viewer().addDockWidget(Qt::RightDockWidgetArea, toolBoxWidget_);
    toolBox_ = new QToolBox(toolBoxWidget_);
    toolBoxWidget_->setWidget(toolBox_);

    auto l = new QFormLayout;

    {
        auto  solType = new QComboBox;
        solType->addItems({"Multidimensional Root Finder", "Multidimensional Minimizer"}); // order must match enum IDs
        solType->setEditable(false);
        solType->setCurrentIndex((*this)->solverSettings().solver_);
        connect(solType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](int ci)
                {
                    auto newsol=insight::cad::ConstrainedSketch::SolverType(ci);
                    auto css=(*this)->solverSettings();
                    if (newsol!=css.solver_)
                    {
                        css.solver_=newsol;
                        (*this)->changeSolverSettings(css);
                        solve();
                    }
                }
                );
        l->addRow("Solver type", solType);
    }
    {
        auto  toledit = new QLineEdit;
        toledit->setValidator(new QDoubleValidator);
        toledit->setText(QString::number((*this)->solverSettings().tolerance_));
        connect(toledit, &QLineEdit::textChanged, this,
                [this](const QString& txt)
                {
                    double newtol = txt.toDouble();
                    auto css=(*this)->solverSettings();
                    if ( fabs(newtol-css.tolerance_) > insight::SMALL)
                    {
                        css.tolerance_=newtol;
                        (*this)->changeSolverSettings(css);
                        solve();
                    }
                }
        );
        l->addRow("Solver tolerance", toledit);
    }
    {
        auto  relaxedit = new QLineEdit;
        relaxedit->setValidator(new QDoubleValidator);
        relaxedit->setText(QString::number((*this)->solverSettings().relax_));
        connect(relaxedit, &QLineEdit::textChanged, this,
                [this](const QString& txt)
                {
                    double newrelax = txt.toDouble();
                    auto css=(*this)->solverSettings();
                    if ( fabs(newrelax-css.relax_) > insight::SMALL)
                    {
                        css.relax_=newrelax;
                        (*this)->changeSolverSettings(css);
                        solve();
                    }
                }
                );
        l->addRow("Solver relaxation", relaxedit);
    }
    {
        auto  maxiteredit = new QLineEdit;
        maxiteredit->setValidator(new QIntValidator(1,INT_MAX));
        maxiteredit->setText(QString::number((*this)->solverSettings().maxIter_));
        connect(maxiteredit, &QLineEdit::textChanged, this,
                [this](const QString& txt)
                {
                    double newmaxiter = txt.toInt();
                    auto css=(*this)->solverSettings();
                    if ( abs(newmaxiter-css.maxIter_) > 0)
                    {
                        css.maxIter_=newmaxiter;
                        (*this)->changeSolverSettings(css);
                        solve();
                    }
                }
                );
        l->addRow("max. iterations", maxiteredit);
    }

    setLayout(l);

    toolBox_->addItem(this, "Sketch");
}




IQVTKConstrainedSketchEditor::~IQVTKConstrainedSketchEditor()
{
    toolBar_->hide();
    toolBar_->deleteLater();
    toolBoxWidget_->hide();
    toolBoxWidget_->deleteLater();

    // copy because "remove" invalidates for-loop
    std::set<insight::cad::ConstrainedSketchEntityPtr> sgs;

    std::transform(sketchGeometryActors_.begin(), sketchGeometryActors_.end(),
        std::inserter(sgs, sgs.end()),
        [](const SketchGeometryActorMap::value_type& v){ return v.first; } );

    for (const auto& sg: sgs)
    {
        remove(sg);
    }
}




insight::cad::ConstrainedSketchEntityPtr
IQVTKConstrainedSketchEditor::findSketchElementOfActor
    (vtkProp *actor) const
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




void IQVTKConstrainedSketchEditor::deleteEntity(std::weak_ptr<insight::cad::ConstrainedSketchEntity> td)
{

    // check if the entity to be deleted is a dependency of any other sketchentity
    std::set<std::comparable_weak_ptr<insight::cad::ConstrainedSketchEntity> > tbd;
    for (auto& e: (*this)->geometry())
    {
        if (e->dependsOn(td))
            tbd.insert(e);
    }
    for (auto& e: tbd)
    {
        deleteEntity(e);
    }

    auto gptr=td.lock();
    remove(gptr);
    (*this)->geometry().erase(gptr);
}




bool IQVTKConstrainedSketchEditor::onKeyRelease ( Qt::KeyboardModifiers modifiers, int key )
{

    bool ret=ViewWidgetAction<IQVTKCADModel3DViewer>
        ::onKeyRelease(modifiers, key);

    if (!ret && (key == Qt::Key_Delete) )
    {
        if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
        {
            if (selact->somethingSelected() && selact->currentSelection().size()>0)
            {
                std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > tbd;
                for (auto& s: selact->currentSelection())
                    tbd.push_back(s);
                selact->clearSelection();
                for (auto& td: tbd)
                {
                    deleteEntity(td);
                }
                ret=true;
            }
        }
    }

    return ret;
}




void IQVTKConstrainedSketchEditor::onMouseMove
(
   Qt::MouseButtons buttons,
   const QPoint point,
   Qt::KeyboardModifiers curFlags
)
{
    ViewWidgetAction<IQVTKCADModel3DViewer>
        ::onMouseMove(buttons, point, curFlags);

    if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
    {
        if (selact->hasSelectionCandidate())
        {
            if (auto sp =
                std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                    selact->currentSelectionCandidate().lock() ) )
            {
                arma::mat pip=viewer().pointInPlane3D(
                    (*this)->plane()->plane(), point );

                arma::mat p2=viewer().pointInPlane2D(
                    (*this)->plane()->plane(), pip );

                sp->setCoords2D(p2(0), p2(1));

                (*this)->invalidate();
                (*this).updateActors();
            }
        }
    }

}





void IQVTKConstrainedSketchEditor::updateActors(int update_msec)
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

  viewer().scheduleRedraw(update_msec);
}
