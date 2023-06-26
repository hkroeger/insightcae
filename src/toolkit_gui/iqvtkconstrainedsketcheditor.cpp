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

#include "iqvtkvieweractions/iqvtkcadmodel3dviewerdrawline.h"
#include "cadpostprocactions/pointdistance.h"
#include "cadpostprocactions/angle.h"
#include "base/parameters/simpleparameter.h"

#include "parametereditorwidget.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>

#include "iqvtkfixedpoint.h"
#include "iqvtkhorizontalconstraint.h"
#include "iqvtkverticalconstraint.h"
#include "iqvtkpointoncurveconstraint.h"

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
    // dimensions etc.
    {
        acs=viewer().createActor(pa);
        setapp=[](const insight::ParameterSet& p, vtkProperty* prop)
            {
                prop->SetColor(0.7, 0.3, 0.3);
                prop->SetLineWidth(0.5);
            };
    }
    else if (auto soe = std::dynamic_pointer_cast<
                   IQVTKConstrainedSketchEntity>(sg))
    // e.g. constrains etc.
    {
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

std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> >
IQVTKConstrainedSketchEditor::findEntitiesUnderCursor(
    const QPoint& point) const
{
    std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > ret;
    auto aa = viewer().findAllActorsUnderCursorAt(point);
    for (auto& a: aa)
    {
        if (auto sg =
                this->findSketchElementOfActor(a))
        {
           ret.push_back(sg);
        }
    }
    return ret;
}




IQVTKCADModel3DViewer::HighlightingHandleSet
IQVTKConstrainedSketchEditor::highlightEntity(
    std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity
    ) const
{
    auto lentity=entity.lock();
    auto sg = sketchGeometryActors_.at(lentity);
    std::set<vtkProp*> props;
    std::copy(
        sg.begin(), sg.end(),
        std::inserter(props, props.begin())
        );
    return viewer().highlightActors(props);
}




void
IQVTKConstrainedSketchEditor::unhighlightEntity(
    IQVTKCADModel3DViewer::HighlightingHandleSet highlighters
    ) const
{
    viewer().unhighlightActors(highlighters);
}





SketchEntityMultiSelection::SketchEntityMultiSelection
( IQVTKConstrainedSketchEditor &editor )
    : editor_(editor)
{
    pe_ = new ParameterEditorWidget(editor_.toolBox_);
    tbi_=editor_.toolBox_->addItem(pe_, "Selection properties");
    editor_.toolBox_->setCurrentIndex(tbi_);

    connect(pe_, &ParameterEditorWidget::parameterSetChanged, pe_,
            [this]()
            {
                for (auto& ee: *this)
                {
                    auto e = ee.first.lock();
                    e->parametersRef().merge(
                        pe_->model()->getParameterSet(),
                        false
                        );
                }
            }
    );

}


SketchEntityMultiSelection::~SketchEntityMultiSelection()
{
    editor_.toolBox_->removeItem(tbi_);
    for (iterator i=begin(); i!=end(); i=begin())
    {
        if (!i->first.expired())
        {
           for (auto& hl: i->second)
           {
               if (!hl.expired())
               {
                   editor_.viewer().unhighlightActor(hl);
               }
           }
        }
        erase(i);
    }
}


void SketchEntityMultiSelection::addToSelection(
    std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity)
{
    if (!isInSelection(entity.lock().get())) // don't add multiple times
    {

        auto i=std::find_if(
                    editor_.sketchGeometryActors_.begin(),
                    editor_.sketchGeometryActors_.end(),
                    [&entity]( const decltype(editor_.sketchGeometryActors_)::value_type& p2 )
            {
                return
                    !entity.owner_before(p2.first)
                 && !p2.first.owner_before(entity);
            }
        );

        if (i!=editor_.sketchGeometryActors_.end())
        {
            mapped_type hl;
            auto actors = i->second;
            for(auto &a: actors)
            {
                hl.insert(editor_.viewer().highlightActor(a));
            }
            insert({entity, hl});

            auto lentity=entity.lock();

            if (size()==1)
            {
                commonParameters_=
                    lentity->parameters();
                defaultCommonParameters_=
                    lentity->defaultParameters();
            }
            else if (size()>1)
            {
                commonParameters_=
                    commonParameters_.intersection(lentity->parameters());
                defaultCommonParameters_=
                    defaultCommonParameters_.intersection(lentity->defaultParameters());
            }

            if (size()>0)
            {
                pe_->clearParameterSet();
                pe_->resetParameterSet(commonParameters_, defaultCommonParameters_);
            }
        }
    }
}

bool SketchEntityMultiSelection::isInSelection(
    const insight::cad::ConstrainedSketchEntity* entity)
{
    for (auto& sel: *this)
    {
        if (sel.first.lock().get()==entity)
            return true;
    }

    return false;
}



void IQVTKConstrainedSketchEditor::drawLine()
{
    auto dl = std::make_shared<IQVTKCADModel3DViewerDrawLine>(viewer(), *this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::updateActors, dl.get(),
            [this]() { updateActors(); } );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::endPointSelected, dl.get(),

            [this]( IQVTKCADModel3DViewerDrawLine::CandidatePoint* addPoint,
                    insight::cad::SketchPointPtr previousPoint )
            {
                if (addPoint->onFeature)
                {
                    (*this)->geometry().insert(
                        IQVTKPointOnCurveConstraint::create( addPoint->sketchPoint, addPoint->onFeature ) ); // fix to curve

                    if (!previousPoint)
                    {
                        // for first point on line: add distance constraint to beginning of line
                        if (auto online = std::dynamic_pointer_cast<insight::cad::Line>(addPoint->onFeature))
                        {
                            double curLen = arma::norm( online->start()->value() - addPoint->sketchPoint->value(), 2);
                            auto lc = insight::cad::DistanceConstraint::create(
                                online->start(), addPoint->sketchPoint,
                                curLen );
                            (*this)->geometry().insert(lc);
                        }
                    }
                }
                else if ( !addPoint->isAnExistingPoint && !previousPoint ) // is first point?
                {
                    (*this)->geometry().insert(
                        IQVTKFixedPoint::create( addPoint->sketchPoint ) ); // fix first point
                }
            }
    );
    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::lineAdded, dl.get(),

            [this]( insight::cad::Line* line,
                    insight::cad::Line* prevLine,
                    IQVTKCADModel3DViewerDrawLine::CandidatePoint* p2,
                    IQVTKCADModel3DViewerDrawLine::CandidatePoint* p1 )
            {
                line->changeDefaultParameters(defaultGeometryParameters_);

                if (!p2->isAnExistingPoint)
                {
                    double curLen = arma::norm( line->start()->value() - line->end()->value(), 2);

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

    currentAction_=dl;
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
    : /*ViewWidgetAction<IQVTKCADModel3DViewer>(viewer),*/
      IQVTKConstrainedSketchEditorSelectionLogic(
        viewer,
        [this]()
        { return std::make_shared<SketchEntityMultiSelection>(*this); }),
      insight::cad::ConstrainedSketchPtr(sketch),
      setActorAppearance_(sseac),
      defaultGeometryParameters_(defaultGeometryParameters)

{
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
            if (currentSelection_)
            {
                for (auto sele: *currentSelection_)
                {
                    auto sg = sele.first.lock();
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
    );
    toolBar_->addAction(
        "V",
        [&]()
        {
            if (currentSelection_)
            {
                for (auto sele: *currentSelection_)
                {
                    auto sg = sele.first.lock();
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
    );

    toolBar_->addAction(
        "X",
        [&]()
        {
            if (currentSelection_)
            {
                insight::cad::SketchPointPtr pt;

                for (auto sele: *currentSelection_)
                {
                    insight::assertion(
                        currentSelection_->size()==1,
                        "exactly one entity should be selected!");

                    auto sg = sele.first.lock();
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
    );

    toolBar_->addAction(
        QPixmap(":/icons/icon_pointoncurve.svg"), "Point on curve",
        [&]()
        {
            if (currentSelection_)
            {
                insight::cad::FeaturePtr curve;
                insight::cad::SketchPointPtr pt;

                for (auto sele: *currentSelection_)
                {
                    auto sg = sele.first.lock();
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
    );

    toolBar_->addAction(
        QPixmap(":/icons/icon_distance.svg"), "Distance",
        [&]()
        {
            if (currentSelection_)
            {
                insight::cad::SketchPointPtr p1, p2;

                if (currentSelection_->size()==2)
                {
                    auto si=currentSelection_->begin();
                    p1 = std::dynamic_pointer_cast<insight::cad::SketchPoint>(    si ->first.lock() );
                    p2 = std::dynamic_pointer_cast<insight::cad::SketchPoint>( (++si)->first.lock() );
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
        );

//    toolBar_->addAction(QPixmap(":/icons/icon_angle.svg"), "Angle",
//                        [&]()
//                        {
//                            if (currentSelection_)
//                            {
//                                insight::cad::SketchPointPtr p1, p2, p3;

//                                if (currentSelection_->size()==2)
//                                {
//                                    auto si=currentSelection_->begin();
//                                    p1 = std::dynamic_pointer_cast<insight::cad::SketchPoint>(    si ->first.lock() );
//                                    p2 = std::dynamic_pointer_cast<insight::cad::SketchPoint>( (++si)->first.lock() );
//                                }

//                                insight::assertion(
//                                    bool(p1)&&bool(p2),
//                                    "Please select exactly two sketch points!");

//                                auto dc = insight::cad::DistanceConstraint::create( p1, p2 );
//                                (*this)->geometry().insert(dc);
//                                (*this)->invalidate();
//                                this->updateActors();
//                            }
//                        });

    toolBar_->addAction(
        QPixmap(":/icons/icon_mergepoints.svg"), "Merge points",
        [&]()
        {
            if (currentSelection_)
            {
                insight::assertion(
                    currentSelection_->size()==2,
                    "Exactly two points have to be selected!");

                auto p1 = std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                                currentSelection_->begin()
                                    ->first.lock());
                insight::assertion(
                    bool(p1), "one entity is not a point!");

                auto p2 = std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                    (++currentSelection_->begin())
                        ->first.lock());
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
    currentSelection_.reset();

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


bool IQVTKConstrainedSketchEditor::onLeftButtonDown  (
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    bool ret=false;

    if (!ret && currentAction_)
        ret=currentAction_->onLeftButtonDown( nFlags, point );

    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

    if (!ret)
    {
        ret=IQVTKConstrainedSketchEditorSelectionLogic::
            onLeftButtonDown(nFlags, point);
    }

    return ret;
}

bool IQVTKConstrainedSketchEditor::onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    bool ret=false;

    if (currentAction_)
        ret=currentAction_->onMiddleButtonDown( nFlags, point );

    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

    return ret;
}

bool IQVTKConstrainedSketchEditor::onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    bool ret=false;

    if (currentAction_)
        ret=currentAction_->onRightButtonDown( nFlags, point );

    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

    if (!ret)
    {
        ret=IQVTKConstrainedSketchEditorSelectionLogic::
            onLeftButtonDown(nFlags, point);
    }

    return ret;
}

bool IQVTKConstrainedSketchEditor::onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    bool ret=false;
    if (currentAction_)
        ret=currentAction_->onLeftButtonUp( nFlags, point );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
    if (!ret)
        ret=IQVTKConstrainedSketchEditorSelectionLogic::
            onLeftButtonUp(nFlags, point);

    return ret;
}

bool IQVTKConstrainedSketchEditor::onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    bool ret=false;

    if (currentAction_)
        ret=currentAction_->onMiddleButtonUp( nFlags, point );

    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

    return ret;
}

bool IQVTKConstrainedSketchEditor::onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    bool ret=false;

    if (currentAction_)
        ret=currentAction_->onRightButtonUp( nFlags, point );

    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

    return ret;
}

bool IQVTKConstrainedSketchEditor::onKeyPress (
    Qt::KeyboardModifiers modifiers, int key )
{
    bool ret=false;

    if (currentAction_)
        ret=currentAction_->onKeyPress( modifiers, key );

    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

    if (!ret)
        ret=IQVTKConstrainedSketchEditorSelectionLogic::
            onKeyPress(modifiers, key);

    return ret;
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
    bool ret=false;
    if (currentAction_)
        ret=currentAction_->onKeyRelease( modifiers, key );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

    if (!ret && (key == Qt::Key_Delete) )
    {
        if (currentSelection_ && currentSelection_->size()>0)
        {
            std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > tbd;
            for (auto& s: *currentSelection_)
                tbd.push_back(s.first);
            currentSelection_.reset();
            for (auto& td: tbd)
            {
                deleteEntity(td);
            }
            ret=true;
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
    if (currentAction_)
        currentAction_->onMouseMove( buttons, point, curFlags );

    if (this->hasSelectionCandidate())
    {
        if (auto sp =
            std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                currentSelectionCandidate().lock() ) )
        {
            arma::mat pip=viewer().pointInPlane3D(
                (*this)->plane()->plane(), point );

            arma::mat p2=viewer().pointInPlane2D(
                (*this)->plane()->plane(), pip );

            sp->setCoords2D(p2(0), p2(1));

            (*this)->invalidate();
            (*this).updateActors();
//            viewer().scheduleRedraw();
        }
    }

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
