#include "iqvtkconstrainedsketcheditor.h"
#include "base/units.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkdragdimensionlineaction.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkLineRepresentation.h"
#include "vtkProperty.h"
#include "vtkSphereSource.h"
#include "vtkVectorText.h"
#include "vtkFollower.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkCaptionActor2D.h"

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewerdrawpoint.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewerdrawline.h"
#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewerdrawrectangle.h"
#include "cadpostprocactions/pointdistance.h"
#include "cadpostprocactions/angle.h"
#include "base/parameters/simpleparameter.h"



#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>
#include <QTableView>

#include "iqvtkconstrainedsketcheditor/iqvtkfixedpoint.h"
#include "iqvtkconstrainedsketcheditor/iqvtkhorizontalconstraint.h"
#include "iqvtkconstrainedsketcheditor/iqvtkverticalconstraint.h"
#include "iqvtkconstrainedsketcheditor/iqvtkpointoncurveconstraint.h"
#include "iqvtkconstrainedsketcheditor/iqconstrainedsketchlayerlistmodel.h"

#include "base/qt5_helper.h"
#include "datum.h"



using namespace insight;
using namespace insight::cad;



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

void IQVTKConstrainedSketchEditor::launchChildAction(Ptr childAction)
{
    childAction->actionIsFinished.connect(
        std::bind(&IQVTKConstrainedSketchEditor::launchDefaultSelectionAction, this) );
    ViewWidgetAction<IQVTKCADModel3DViewer>::launchChildAction(childAction);
}



void IQVTKConstrainedSketchEditor::launchDefaultSelectionAction()
{
    auto sel = std::make_shared<IQVTKSelectConstrainedSketchEntity>(*this);
    ViewWidgetAction<IQVTKCADModel3DViewer>::launchChildAction(sel);
}




void IQVTKConstrainedSketchEditor::drawPoint()
{
    auto dl = std::make_shared<IQVTKCADModel3DViewerDrawPoint>(*this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawPoint::pointAdded, dl.get(),

            [this]( IQVTKCADModel3DViewerDrawPoint::PointProperty pp )
            {
                if (pp.onFeature)
                {
                    if (auto online =
                        std::dynamic_pointer_cast<insight::cad::Line>(
                            pp.onFeature ) )
                    {
                        (*this)->insertGeometry(
                            IQVTKPointOnCurveConstraint::create(
                                pp.p,
                                online ) ); // fix to curve

                        (*this)->insertGeometry(
                            insight::cad::DistanceConstraint::create(
                                online->start(), pp.p,
                                (*this)->sketchPlaneNormal() ) );
                    }
                }
                else
                {
                    (*this)->insertGeometry(
                        IQVTKFixedPoint::create( pp.p ) ); // fix first point
                }

                (*this)->invalidate();
                this->updateActors();
                Q_EMIT sketchChanged();
            }
    );



    launchChildAction(dl);
}



void IQVTKConstrainedSketchEditor::drawLine()
{
    auto dl = std::make_shared<IQVTKCADModel3DViewerDrawLine>(*this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::updateActors, dl.get(),
            [this]() { updateActors(); } );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::endPointSelected, dl.get(),

            [this]( IQVTKCADModel3DViewerDrawLine::PointProperty* addPoint,
                   insight::cad::SketchPointPtr previousPoint )
            {
                if ( !(addPoint->isAnExistingPoint||addPoint->onFeature) && !previousPoint ) // is first point?
                {
                    (*this)->insertGeometry(
                        IQVTKFixedPoint::create( addPoint->p ) ); // fix it
                }

                else if (addPoint->onFeature)
                {
                    if (auto online =
                        std::dynamic_pointer_cast<insight::cad::Line>(
                            addPoint->onFeature ) )
                    {
                        // add point-on-curve constraint
                        (*this)->insertGeometry(
                            IQVTKPointOnCurveConstraint::create(
                                addPoint->p,
                                addPoint->onFeature ) ); // fix to curve

                        // constrain distance to start of hit line
                        (*this)->insertGeometry(
                            insight::cad::DistanceConstraint::create(
                                online->start(), addPoint->p,
                                (*this)->sketchPlaneNormal() ) );
                    }
                }
            }
            );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::lineAdded, dl.get(),

            [this]( std::shared_ptr<insight::cad::Line> line,
                   insight::cad::Line* prevLine,
                   IQVTKCADModel3DViewerDrawLine::PointProperty* p2,
                   IQVTKCADModel3DViewerDrawLine::PointProperty* p1 )
            {
                line->changeDefaultParameters(defaultGeometryParameters_);

                auto pointIsFixed = [](IQVTKCADModel3DViewerDrawLine::PointProperty *pp)
                {
                    return pp->isAnExistingPoint || bool(pp->onFeature);
                };

                bool isLastLine = p2->isAnExistingPoint; // closed

                if (!(pointIsFixed(p1) && pointIsFixed(p2)) && !isLastLine)
                {
                    (*this)->insertGeometry(
                        insight::cad::DistanceConstraint::create(
                            line->start(), line->end(),
                            (*this)->sketchPlaneNormal() ) );

                    // add angle constraint
                    if (prevLine)
                    {
                        (*this)->insertGeometry(
                            insight::cad::AngleConstraint::create(
                                prevLine->start(),
                                line->end(), line->start() ) );
                    }
                    else
                    {
                        auto p1 = std::make_shared<insight::cad::AddedVector>(
                            line->start(),
                            insight::cad::vec3const(1,0,0) );

                        double ang = insight::cad::AngleConstraint::calculate(
                            p1->value(),
                            line->end()->value(),
                            line->start()->value() );

                        if (
                            fabs(ang)<insight::SMALL
                            ||fabs(ang-M_PI)<insight::SMALL
                            )
                        {
                            (*this)->insertGeometry(
                                IQVTKHorizontalConstraint::create(
                                    line ) );
                        }
                        else if (
                            (fabs(ang-0.5*M_PI)<insight::SMALL)
                            ||(fabs(ang+0.5*M_PI)<insight::SMALL)
                            )
                        {
                            (*this)->insertGeometry(
                                IQVTKVerticalConstraint::create(
                                    line ) );
                        }
                        else
                        {
                            (*this)->insertGeometry(
                                insight::cad::AngleConstraint::create(
                                    p1,
                                    line->end(), line->start() ) );
                        }
                    }
                }


                (*this)->invalidate();
                this->updateActors();
                Q_EMIT sketchChanged();
            }
            );

    launchChildAction(dl);
}




void IQVTKConstrainedSketchEditor::drawRectangle()
{
    auto dl = std::make_shared<IQVTKCADModel3DViewerDrawRectangle>(*this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawRectangle::rectangleAdded, dl.get(),

            [this]( std::vector<std::shared_ptr<insight::cad::Line> > addedLines,
                    IQVTKCADModel3DViewerDrawRectangle::PointProperty* p2,
                    IQVTKCADModel3DViewerDrawRectangle::PointProperty* p1 )
            {
                if (p1->onFeature)
                {
                    if (auto online =
                        std::dynamic_pointer_cast<insight::cad::Line>(
                            p1->onFeature ) )
                    {
                        (*this)->insertGeometry(
                            IQVTKPointOnCurveConstraint::create(
                                p1->p,
                                online ) ); // fix to curve

                        (*this)->insertGeometry(
                            insight::cad::DistanceConstraint::create(
                                online->start(), p1->p,
                                (*this)->sketchPlaneNormal() ) );
                    }
                }
                else
                {
                    (*this)->insertGeometry(
                        IQVTKFixedPoint::create( p1->p ) ); // fix first point
                }

                if (p2->onFeature)
                {
                    if (auto online =
                        std::dynamic_pointer_cast<insight::cad::Line>(
                            p2->onFeature ) )
                    {
                        (*this)->insertGeometry(
                            IQVTKPointOnCurveConstraint::create(
                                p2->p,
                                online ) ); // fix to curve

                        (*this)->insertGeometry(
                            insight::cad::DistanceConstraint::create(
                                online->start(), p2->p,
                                (*this)->sketchPlaneNormal() ) );
                    }
                }
                else
                {
                    (*this)->insertGeometry(
                        insight::cad::DistanceConstraint::create(
                            addedLines[0]->start(), addedLines[0]->end(),
                            (*this)->sketchPlaneNormal() ) );
                    (*this)->insertGeometry(
                        insight::cad::DistanceConstraint::create(
                            addedLines[1]->start(), addedLines[1]->end(),
                            (*this)->sketchPlaneNormal() ) );
                }

                (*this)->insertGeometry(
                    IQVTKHorizontalConstraint::create( addedLines[0] )
                    );
                (*this)->insertGeometry(
                    IQVTKHorizontalConstraint::create( addedLines[2] )
                    );
                (*this)->insertGeometry(
                    IQVTKVerticalConstraint::create( addedLines[1] )
                    );
                (*this)->insertGeometry(
                    IQVTKVerticalConstraint::create( addedLines[3] )
                    );

                for (auto& line: addedLines)
                {
                    line->changeDefaultParameters(defaultGeometryParameters_);
                }

                (*this)->invalidate();
                this->updateActors();
                Q_EMIT sketchChanged();
            }
            );



    launchChildAction(dl);
}




void IQVTKConstrainedSketchEditor::solve()
{
    (*this)->resolveConstraints(
//        [&]()
//        {
//            this->updateActors();
//        }
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
      insight::cad::ConstrainedSketchPtr(sketch),
      setActorAppearance_(sseac),
      defaultGeometryParameters_(defaultGeometryParameters)

{

    toolBar_ = this->viewer().addToolBar("Sketcher commands");

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawpoint.svg"), "Point",
                        this, &IQVTKConstrainedSketchEditor::drawPoint);

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawline.svg"), "Line",
                        this, &IQVTKConstrainedSketchEditor::drawLine);

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawrectangle.svg"), "Rectangle",
                        this, &IQVTKConstrainedSketchEditor::drawRectangle);

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_solve.svg"), "Solve",
                        this, &IQVTKConstrainedSketchEditor::solve);

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_finish.svg"), "Finish",
                        std::bind(&IQVTKConstrainedSketchEditor::finishAction, this) );

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
                            (*this)->insertGeometry(lc);
                            (*this)->invalidate();
                            this->updateActors();
                            Q_EMIT sketchChanged();
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
                            (*this)->insertGeometry(lc);
                            (*this)->invalidate();
                            this->updateActors();
                            Q_EMIT sketchChanged();
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
                            (*this)->insertGeometry(c);
                            (*this)->invalidate();
                            this->updateActors();
                            Q_EMIT sketchChanged();
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
                            (*this)->insertGeometry(lc);
                            (*this)->invalidate();
                            this->updateActors();
                            Q_EMIT sketchChanged();
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

                    auto dc = insight::cad::DistanceConstraint::create(
                        p1, p2,
                        (*this)->sketchPlaneNormal() );
                    (*this)->insertGeometry(dc);
                    (*this)->invalidate();
                    this->updateActors();
                    Q_EMIT sketchChanged();
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

                    for (auto& e: **this)
                    {
                        e.second->replaceDependency(p1, p2);
                    }
                    (*this)->eraseGeometry(p1);
                    (*this)->invalidate();
                    this->updateActors();
                    Q_EMIT sketchChanged();
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
                for (auto& geom: **this)
                {
                    geom.second->scaleSketch(sf);
                }
                (*this)->invalidate();
                this->updateActors();
                Q_EMIT sketchChanged();
            }
        }
    );
    toolBar_->show();


    // this editor is its own property widget

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
                        Q_EMIT sketchChanged();
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
                        Q_EMIT sketchChanged();
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
                        Q_EMIT sketchChanged();
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
                        Q_EMIT sketchChanged();
                    }
                }
                );
        l->addRow("max. iterations", maxiteredit);
    }
    {
        auto  layerlist = new QTableView;
        auto *model=new IQConstrainedSketchLayerListModel(this, this);
        model->update();
        connect(this, &IQVTKConstrainedSketchEditor::sketchChanged,
                model, &IQConstrainedSketchLayerListModel::update);
        connect(model, &IQConstrainedSketchLayerListModel::hideLayer,
                this, &IQVTKConstrainedSketchEditor::hideLayer);
        connect(model, &IQConstrainedSketchLayerListModel::showLayer,
                this, &IQVTKConstrainedSketchEditor::showLayer);
        connect(model, &IQConstrainedSketchLayerListModel::renameLayer,
                this, &IQVTKConstrainedSketchEditor::renameLayer);
        layerlist->setModel(model);
        l->addRow("Layers", layerlist);
    }
    setLayout(l);

    viewer.commonToolBox()->addItem(this, "Sketch");
}




IQVTKConstrainedSketchEditor::~IQVTKConstrainedSketchEditor()
{
    toolBar_->hide();
    toolBar_->deleteLater();

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



void IQVTKConstrainedSketchEditor::start()
{
    transparency_.reset(
        new IQVTKCADModel3DViewer::ExposeItem(
            nullptr, QModelIndex(), viewer() )
        );
    launchDefaultSelectionAction();
    updateActors();
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
    for (auto& e: **this)
    {
        if (e.second->dependsOn(td))
            tbd.insert(e.second);
    }
    for (auto& e: tbd)
    {
        deleteEntity(e);
    }

    auto gptr=td.lock();
    remove(gptr);
    (*this)->eraseGeometry(gptr);
}

bool IQVTKConstrainedSketchEditor::layerIsVisible(const std::string &layerName) const
{
    return hiddenLayers_.count(layerName)==0;
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




bool IQVTKConstrainedSketchEditor::onMouseMove
(
   Qt::MouseButtons buttons,
   const QPoint point,
   Qt::KeyboardModifiers curFlags
)
{
    bool ret = ViewWidgetAction<IQVTKCADModel3DViewer>
        ::onMouseMove(buttons, point, curFlags);

    if (!ret)
    {
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

                    ret=true;
                }
                else if (auto dc =
                    std::dynamic_pointer_cast<insight::cad::DistanceConstraint>(
                        selact->currentSelectionCandidate().lock() ) )
                {
                    launchChildAction(std::make_shared<IQVTKDragDimensionlineAction>(*this, dc));

                    ret=true;
                }
            }
        }
    }

    return ret;
}





void IQVTKConstrainedSketchEditor::updateActors()
{

  auto bb=(*this)->sketchBoundingBox();
  double Ldiag=arma::norm(bb.col(1)-bb.col(0), 2);
  for (const auto& g: **this)
  {
        if (auto dim =
            std::dynamic_pointer_cast<DistanceConstraint>(g.second))
        {
            auto &asp = dim->parametersRef().get<DoubleParameter>("arrowSize");
            asp.set(Ldiag*0.01, true);
        }
  }


  // through all geom elements
  for (const auto& g: **this)
  {
      // remove, if displayed already
      auto i=sketchGeometryActors_.find(g.second);
      if (i!=sketchGeometryActors_.end())
      {
          remove(g.second);
      }

      // add, if not filtered
      if (hiddenLayers_.count(g.second->layerName())==0)
        add(g.second);
  }

  for (const auto& sga: sketchGeometryActors_)
  {
      auto g=sga.first;
      auto i=(*this)->findGeometry(g);
      if (i==(*this)->end())
      {
        remove(g);
      }
  }

  viewer().scheduleRedraw();
}




void IQVTKConstrainedSketchEditor::hideLayer(const std::string &layerName)
{
  if (layerIsVisible(layerName))
  {
      hiddenLayers_.insert(layerName);
      updateActors();
  }
}

void IQVTKConstrainedSketchEditor::showLayer(const std::string &layerName)
{
  if (!layerIsVisible(layerName))
  {
      hiddenLayers_.erase(layerName);
      updateActors();
  }
}

void IQVTKConstrainedSketchEditor::renameLayer(
    const std::string &currentLayerName,
    const std::string &newLayerName )
{
  for (auto& g: **this)
  {
      auto feat=g.second;
      if (feat->layerName()==currentLayerName)
        feat->setLayerName(newLayerName);
  }
  if (hiddenLayers_.count(currentLayerName))
  {
      hiddenLayers_.erase(currentLayerName);
      hiddenLayers_.insert(newLayerName);
  }
  Q_EMIT sketchChanged();
}
