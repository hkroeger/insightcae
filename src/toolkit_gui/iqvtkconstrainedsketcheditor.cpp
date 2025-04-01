#include "iqvtkconstrainedsketcheditor.h"
#include "base/exception.h"
#include "base/units.h"
#include "constrainedsketch.h"
#include "iqvtkconstrainedsketcheditor/iqvtkdragdimensionlineaction.h"
#include "iqvtkconstrainedsketcheditor/iqvtkdragangledimensionaction.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqparametersetmodel.h"
#include "parametereditorwidget.h"

#include "vtkLineRepresentation.h"
#include "vtkProperty.h"
#include "vtkSphereSource.h"
#include "vtkVectorText.h"
#include "vtkFollower.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkCaptionActor2D.h"

#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerdrawpoint.h"
#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerdrawline.h"
#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerdrawrectangle.h"
#include "base/parameters/simpleparameter.h"



#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QDoubleValidator>
#include <QComboBox>
#include <QTableView>
#include <qnamespace.h>

#include "constrainedsketchentities/distanceconstraint.h"
#include "constrainedsketchentities/angleconstraint.h"
#include "constrainedsketchentities/fixedpointconstraint.h"
#include "constrainedsketchentities/horizontalconstraint.h"
#include "constrainedsketchentities/verticalconstraint.h"
#include "constrainedsketchentities/pointoncurveconstraint.h"
#include "iqvtkconstrainedsketcheditor/iqconstrainedsketchlayerlistmodel.h"
#include "iqvtkconstrainedsketcheditor/iqconstrainedsketchentitylistmodel.h"

#include "base/qt5_helper.h"
#include "datum.h"
#include "cadfeatures/singleedgefeature.h"


using namespace insight;
using namespace insight::cad;


defineType(DefaultGUIConstrainedSketchPresentationDelegate);
insight::cad::ConstrainedSketchPresentationDelegate::Add<DefaultGUIConstrainedSketchPresentationDelegate>
    addDefaultGUIConstrainedSketchPresentationDelegate;

std::string defaultGUIConstrainedSketchPresentationDelegate(
    DefaultGUIConstrainedSketchPresentationDelegate::typeName );



IQParameterSetModel*
DefaultGUIConstrainedSketchPresentationDelegate::setupSketchEntityParameterSetModel(
    const insight::cad::ConstrainedSketchEntity& e) const
{
    if (e.parameters().size()>0)
        return new IQParameterSetModel(
            e.parameters().cloneParameterSet());
    else
        return nullptr;
}


IQParameterSetModel*
DefaultGUIConstrainedSketchPresentationDelegate::setupLayerParameterSetModel(
    const std::string& layerName, const insight::cad::LayerProperties& curP) const
{
    if (curP.size()>0)
    {
        return new IQParameterSetModel(
            curP.cloneParameterSet());
    }
    else
    {
        return nullptr;
    }
}


void DefaultGUIConstrainedSketchPresentationDelegate::setEntityAppearance(
    const insight::cad::ConstrainedSketchEntity &e, vtkProperty *actprops ) const
{
    if (dynamic_cast<const insight::cad::Feature*>(&e))
    {
        auto sec = QColorConstants::DarkCyan;
        actprops->SetColor(
            sec.redF(),
            sec.greenF(),
            sec.blueF() );
        actprops->SetLineWidth(2);
    }
    else
    {
        auto sec = QColorConstants::Black;
        actprops->SetColor(
            sec.redF(),
            sec.greenF(),
            sec.blueF() );
        actprops->SetLineWidth(1);
    }
}


void IQVTKConstrainedSketchEditor::showLayerParameterEditor()
{
    if (!layerPropertiesEditor_)
    {
        auto *pew = new QWidget;
        auto *lo = new QVBoxLayout;
        pew->setLayout(lo);

        auto tree=new QTreeView;
        lo->addWidget(tree);
        auto editControls = new QWidget;
        lo->addWidget(editControls);

        layerPropertiesEditor_ = new ParameterEditorWidget(pew, tree, editControls, &viewer());

        auto *l = static_cast<QFormLayout*>(sketchToolBoxWidget_->layout());
        l->addRow(pew);
    }
}


void IQVTKConstrainedSketchEditor::hideLayerParameterEditor()
{
    if (layerPropertiesEditor_)
    {
        delete layerPropertiesEditor_->parent();
        layerPropertiesEditor_=nullptr;
    }
}


void IQVTKConstrainedSketchEditor::add(
    ConstrainedSketchEntityPtr sg)
{
    std::vector<vtkSmartPointer<vtkProp> > acs;

    if (auto feat = std::dynamic_pointer_cast<
            Feature>(sg))
    {
        acs=viewer().createActor(feat);
    }
    else if (auto pt = std::dynamic_pointer_cast<
                   insight::cad::Vector>(sg))
    {
        acs=viewer().createActor(pt);
    }
    // else if (auto pa = std::dynamic_pointer_cast<
    //                PostprocAction>(sg))
    // {
    //     // dimensions etc.
    //     acs=viewer().createActor(pa);
    //     setapp=[](const insight::ParameterSet& p, vtkProperty* prop)
    //         {
    //             prop->SetColor(0.7, 0.3, 0.3);
    //             prop->SetLineWidth(0.5);
    //         };
    // }
    else if (auto soe = std::dynamic_pointer_cast<
                   ConstrainedSketchEntity>(sg))
    {
        // e.g. constrains etc.
        acs=soe->createActor();
    }

    ActorSet as;
    for (auto& a: acs)
    {
        if (vtkSmartPointer<vtkActor> aa = vtkActor::SafeDownCast(a))
        {
            presentation_->setEntityAppearance(*sg, aa->GetProperty());
        }
        if (auto follower=vtkFollower::SafeDownCast(a))
        {
            follower->SetCamera(
                viewer().renderer()->GetActiveCamera());
        }
        viewer().renderer()->AddActor(a);
        as.insert(a);
    }

    sketchGeometryActors_[sg]=as;
}




void IQVTKConstrainedSketchEditor::remove(
    ConstrainedSketchEntityPtr sg)
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




IQVTKConstrainedSketchEditor::ViewWidgetActionHost::ViewWidgetActionPtr
IQVTKConstrainedSketchEditor::setupDefaultAction()
{
    return make_viewWidgetAction<IQVTKSelectConstrainedSketchEntity>(*this, true);
}





void IQVTKConstrainedSketchEditor::drawPoint()
{
    auto dl = make_viewWidgetAction<IQVTKCADModel3DViewerDrawPoint>(*this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawPoint::pointAdded, dl.get(),

            [this]( IQVTKCADModel3DViewerDrawPoint::PointProperty pp )
            {
                if (pp.onFeature)
                {
                    if ( pp.onFeature
                            ->topologicalProperties().onlyEdges() )
                    {
                        (*this)->insertGeometry(
                            PointOnCurveConstraint::create(
                                pp.p,
                                pp.onFeature ) ); // fix to curve

                        if (auto edg = std::dynamic_pointer_cast<SingleEdgeFeature>(
                                pp.onFeature))
                        {
                            (*this)->insertGeometry(
                                FixedDistanceConstraint::create(
                                    edg->start(), pp.p,
                                    (*this)->sketchPlaneNormal() ) );
                        }
                    }
                }
                else
                {
                    (*this)->insertGeometry(
                        FixedPointConstraint::create( pp.p ) ); // fix first point
                }

                entityProperties_->changeDefaultParameters(*pp.p);

                (*this)->invalidate();
                this->updateActors();
                Q_EMIT sketchChanged();
            }
    );



    launchAction(std::move(dl));
}



void IQVTKConstrainedSketchEditor::drawLine()
{
    auto dl = make_viewWidgetAction<IQVTKCADModel3DViewerDrawLine>(*this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::updateActors, dl.get(),
            [this]() { updateActors(); } );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::endPointSelected, dl.get(),

            [this]( IQVTKCADModel3DViewerDrawLine::PointProperty* addPoint,
                   SketchPointPtr previousPoint )
            {
                if ( !(addPoint->isAnExistingPoint||addPoint->onFeature) && !previousPoint ) // is first point?
                {
                    (*this)->insertGeometry(
                        FixedPointConstraint::create( addPoint->p ) ); // fix it
                }

                else if (addPoint->onFeature)
                {
                    if ( addPoint->onFeature
                            ->topologicalProperties().onlyEdges() )
                    {
                        // add point-on-curve constraint
                        (*this)->insertGeometry(
                            PointOnCurveConstraint::create(
                                addPoint->p,
                                addPoint->onFeature ) ); // fix to curve

                        if (auto online =
                            std::dynamic_pointer_cast<SingleEdgeFeature>(
                                addPoint->onFeature ) )
                        {
                            // constrain distance to start of hit line
                            (*this)->insertGeometry(
                                FixedDistanceConstraint::create(
                                    online->start(), addPoint->p,
                                    (*this)->sketchPlaneNormal() ) );
                        }
                    }
                }
            }
            );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::lineAdded, dl.get(),

            [this]( std::shared_ptr<Line> line,
                   Line* prevLine,
                   IQVTKCADModel3DViewerDrawLine::PointProperty* p2,
                   IQVTKCADModel3DViewerDrawLine::PointProperty* p1 )
            {
                entityProperties_->changeDefaultParameters(*line);

                auto pointIsFixed = [](IQVTKCADModel3DViewerDrawLine::PointProperty *pp)
                {
                    return pp->isAnExistingPoint || bool(pp->onFeature);
                };

                bool isLastLine = p2->isAnExistingPoint; // closed

                if (!(pointIsFixed(p1) && pointIsFixed(p2)) && !isLastLine)
                {
                    (*this)->insertGeometry(
                        FixedDistanceConstraint::create(
                            line->start(), line->end(),
                            (*this)->sketchPlaneNormal() ) );

                    // add angle constraint
                    if (prevLine)
                    {
                        (*this)->insertGeometry(
                            FixedAngleConstraint::create(
                                prevLine->start(), line->end(), line->start() ) );
                    }
                    else
                    {
                        double ang = AngleConstraint::calculate(
                            line->start()->value()+insight::vec3X(1),
                            line->end()->value(),
                            line->start()->value() );

                        if (
                            fabs(ang)<insight::SMALL
                            ||fabs(ang-M_PI)<insight::SMALL
                            )
                        {
                            (*this)->insertGeometry(
                                HorizontalConstraint::create(
                                    line ) );
                        }
                        else if (
                            (fabs(ang-0.5*M_PI)<insight::SMALL)
                            ||(fabs(ang+0.5*M_PI)<insight::SMALL)
                            )
                        {
                            (*this)->insertGeometry(
                                VerticalConstraint::create(
                                    line ) );
                        }
                        else
                        {
                            (*this)->insertGeometry(
                                FixedAngleConstraint::create(
                                    line->end(), nullptr, line->start() ) );
                        }
                    }
                }


                (*this)->invalidate();
                this->updateActors();
                Q_EMIT sketchChanged();
            }
            );

    launchAction(std::move(dl));
}




void IQVTKConstrainedSketchEditor::drawRectangle()
{
    auto dl = make_viewWidgetAction<IQVTKCADModel3DViewerDrawRectangle>(*this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawRectangle::rectangleAdded, dl.get(),

            [this]( std::vector<std::shared_ptr<Line> > addedLines,
                    IQVTKCADModel3DViewerDrawRectangle::PointProperty* p2,
                    IQVTKCADModel3DViewerDrawRectangle::PointProperty* p1 )
            {
                if (p1->onFeature)
                {
                    if (auto online =
                        std::dynamic_pointer_cast<Line>(
                            p1->onFeature ) )
                    {
                        (*this)->insertGeometry(
                            PointOnCurveConstraint::create(
                                p1->p,
                                online ) ); // fix to curve

                        (*this)->insertGeometry(
                            FixedDistanceConstraint::create(
                                online->start(), p1->p,
                                (*this)->sketchPlaneNormal() ) );
                    }
                }
                else
                {
                    (*this)->insertGeometry(
                        FixedPointConstraint::create( p1->p ) ); // fix first point
                }

                if (p2->onFeature)
                {
                    if (auto online =
                        std::dynamic_pointer_cast<Line>(
                            p2->onFeature ) )
                    {
                        (*this)->insertGeometry(
                            PointOnCurveConstraint::create(
                                p2->p,
                                online ) ); // fix to curve

                        (*this)->insertGeometry(
                            FixedDistanceConstraint::create(
                                online->start(), p2->p,
                                (*this)->sketchPlaneNormal() ) );
                    }
                }
                else
                {
                    (*this)->insertGeometry(
                        FixedDistanceConstraint::create(
                            addedLines[0]->start(), addedLines[0]->end(),
                            (*this)->sketchPlaneNormal() ) );
                    (*this)->insertGeometry(
                        FixedDistanceConstraint::create(
                            addedLines[1]->start(), addedLines[1]->end(),
                            (*this)->sketchPlaneNormal() ) );
                }

                (*this)->insertGeometry(
                    HorizontalConstraint::create( addedLines[0] )
                    );
                (*this)->insertGeometry(
                    HorizontalConstraint::create( addedLines[2] )
                    );
                (*this)->insertGeometry(
                    VerticalConstraint::create( addedLines[1] )
                    );
                (*this)->insertGeometry(
                    VerticalConstraint::create( addedLines[3] )
                    );

                for (auto& line: addedLines)
                {
                    entityProperties_->changeDefaultParameters(*line);
                }

                (*this)->invalidate();
                this->updateActors();
                Q_EMIT sketchChanged();
            }
            );



    launchAction(std::move(dl));
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
    const insight::cad::ConstrainedSketch& sketch,
    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties,
    const std::string& presentationDelegateKey
)
: ViewWidgetAction<IQVTKCADModel3DViewer>(viewer, false),
  insight::cad::ConstrainedSketchPtr(
      insight::cad::ConstrainedSketch::create<const ConstrainedSketch&>(sketch)),
  entityProperties_(
          entityProperties?
              entityProperties : std::make_shared<insight::cad::ConstrainedSketchParametersDelegate>() ),
  layerPropertiesEditor_(nullptr)
{
    if (!presentationDelegateKey.empty())
    {
        presentation_=
            insight::cad::ConstrainedSketchPresentationDelegate::delegates()(
              presentationDelegateKey );
    }
    if (!presentation_)
    {
        presentation_ =
            std::make_shared<DefaultGUIConstrainedSketchPresentationDelegate>();
    }

    toolBar_ = this->viewer().addToolBar("Sketcher commands");

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_finish_accept.svg"), "Finish & Accept",
                        this, std::bind(&IQVTKConstrainedSketchEditor::finishAction, this, true) );

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_finish_cancel.svg"), "Cancel",
                        this, std::bind(&IQVTKConstrainedSketchEditor::finishAction, this, false) );

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawpoint.svg"), "Point",
                        this, &IQVTKConstrainedSketchEditor::drawPoint);

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawline.svg"), "Line",
                        this, &IQVTKConstrainedSketchEditor::drawLine);

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawrectangle.svg"), "Rectangle",
                        this, &IQVTKConstrainedSketchEditor::drawRectangle);

    toolBar_->addAction(QPixmap(":/icons/icon_sketch_solve.svg"), "Solve",
                        this, &IQVTKConstrainedSketchEditor::solve);

    toolBar_->addAction(
        "H",
        this, [this]()
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
                            auto lc = HorizontalConstraint::create( l );
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
        this, [this]()
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
                            auto lc = insight::cad::VerticalConstraint::create( l );
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
        QPixmap(":/icons/icon_fixpoint.svg"), "Fix Point Coordinates",
        this, [this]()
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
                            auto c = FixedPointConstraint::create( p );
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
        QPixmap(":/icons/icon_distance_xf.svg"), "Fix Points X-Coordinate",
        this, [this]()
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
                            auto c = FixedDistanceConstraint::create(
                                vec3const(0,0,0), p,
                                (*this)->sketchPlaneNormal(),
                                std::string(),
                                vec3const(1,0,0) );
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
        QPixmap(":/icons/icon_distance_yf.svg"), "Fix Points Y-Coordinate",
        this, [this]()
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
                            auto c = FixedDistanceConstraint::create(
                                vec3const(0,0,0), p,
                                (*this)->sketchPlaneNormal(),
                                std::string(),
                                vec3const(0,1,0) );
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
        this, [this]()
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
                        if (auto p =
                            std::dynamic_pointer_cast<
                                insight::cad::SketchPoint>(sg))
                        {
                            pt=p;
                        }
                        else if (auto l =
                                   std::dynamic_pointer_cast<
                                       insight::cad::Feature>(sg))
                        {
                            if (l->topologicalProperties().onlyEdges())
                            {
                                curve=l;
                            }
                        }

                        if (pt && curve)
                        {
                            auto lc = PointOnCurveConstraint::create( pt, curve );
                            (*this)->insertGeometry(lc);
                            (*this)->invalidate();
                            this->updateActors();
                            Q_EMIT sketchChanged();
                            break;
                        }
                    }
                }
            }
        }
    );

    toolBar_->addAction(
        QPixmap(":/icons/icon_distance.svg"), "Distance",
        this, [this]()
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
                    else if (selact->currentSelection().size()==1)
                    {
                        auto si=selact->currentSelection().begin();
                        if (auto l = std::dynamic_pointer_cast<cad::SingleEdgeFeature>(si->lock()))
                        {
                            p1 = std::dynamic_pointer_cast<insight::cad::SketchPoint>( l->start() );
                            p2 = std::dynamic_pointer_cast<insight::cad::SketchPoint>( l->end() );
                        }
                    }

                    insight::assertion(
                        bool(p1)&&bool(p2),
                        "Please select exactly two sketch points or a single line!");

                    auto dc = insight::cad::FixedDistanceConstraint::create(
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
        this, [this]()
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
        this, [this]()
        {
            bool ok;
            double sf=QInputDialog::getDouble(
                &this->viewer(),
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

        connect(
            layerlist->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            [this,model](const QModelIndex &current, const QModelIndex &previous)
            {
                if (current.isValid())
                {
                    auto selectedLayerName =
                        model->data(current.siblingAtColumn(1))
                                             .toString().toStdString();

                    if (auto m= presentation_->setupLayerParameterSetModel(
                            selectedLayerName,
                            this->sketch().layerProperties(selectedLayerName)))
                    {

                        showLayerParameterEditor();
                        layerPropertiesEditor_->setModel(m);

                        connect(
                            layerPropertiesEditor_,
                            &ParameterEditorWidget::parameterSetChanged,
                            m,
                            [this,m,model,selectedLayerName]()
                            {
                                (*this)->setLayerProperties(
                                        selectedLayerName,
                                        m->getParameterSet()
                                    );
                            }
                            );
                    }
                    else
                    {
                        hideLayerParameterEditor();
                    }
                }
                else
                {
                    hideLayerParameterEditor();
                }
            }
            );

        l->addRow("Layers", layerlist);
    }


    {
        auto  geolist = new QTableView;
        auto *model=new IQConstrainedSketchEntityListModel(this, this);
        model->update();
        connect(this, &IQVTKConstrainedSketchEditor::sketchChanged,
                model, &IQConstrainedSketchEntityListModel::update);
        // connect(model, &IQConstrainedSketchLayerListModel::hideLayer,
        //         this, &IQVTKConstrainedSketchEditor::hideLayer);
        // connect(model, &IQConstrainedSketchLayerListModel::showLayer,
        //         this, &IQVTKConstrainedSketchEditor::showLayer);
        // connect(model, &IQConstrainedSketchLayerListModel::renameLayer,
        //         this, &IQVTKConstrainedSketchEditor::renameLayer);
        geolist->setModel(model);

        connect(
            geolist->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [this,model](
                const QItemSelection &selected,
                const QItemSelection &deselected )
            {
                if (auto*selection = runningAction<IQVTKSelectConstrainedSketchEntity>())
                {
                    for (auto desel: deselected.indexes())
                    {
                        std::weak_ptr<insight::cad::ConstrainedSketchEntity> e
                            = (*this)->get<insight::cad::ConstrainedSketchEntity>(
                                desel.siblingAtColumn(0).data().toInt());
                        selection->externallyUnselect(e);
                    }
                    for (auto sel: selected.indexes())
                    {
                        std::weak_ptr<insight::cad::ConstrainedSketchEntity> e
                            = (*this)->get<insight::cad::ConstrainedSketchEntity>(
                                sel.siblingAtColumn(0).data().toInt());
                        selection->externallySelect(e);
                    }
                }
            }
            );


        l->addRow("Entities", geolist);
    }


    sketchToolBoxWidget_=new QWidget;
    sketchToolBoxWidget_->setLayout(l);
    viewer.addToolBox(sketchToolBoxWidget_, "Sketch");

    aboutToBeDestroyed.connect(
        [this](){
            cancelCurrentAction();

            transparency_.reset();

            {
                // copy because "remove" invalidates for-loop
                std::set<insight::cad::ConstrainedSketchEntityPtr> sgs;

                std::transform(sketchGeometryActors_.begin(), sketchGeometryActors_.end(),
                               std::inserter(sgs, sgs.end()),
                               [](const SketchGeometryActorMap::value_type& v){ return v.first; } );

                for (const auto& sg: sgs)
                {
                    remove(sg);
                }

                sketchGeometryActors_.clear();
            }

            // tbw->hide();
            // tbw->deleteLater();
            // toolBar_->hide();
            // toolBar_->deleteLater();
            delete sketchToolBoxWidget_;
            delete toolBar_;
        });
}




IQVTKConstrainedSketchEditor::~IQVTKConstrainedSketchEditor()
{}



QString IQVTKConstrainedSketchEditor::description() const
{
    return "Edit sketch";
}



void IQVTKConstrainedSketchEditor::start()
{
    transparency_.reset(
        new IQVTKCADModel3DViewer::ExposeItem(
            nullptr, QModelIndex(), viewer() )
        );
    setDefaultAction();
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
    Q_EMIT sketchChanged();
}




bool IQVTKConstrainedSketchEditor::layerIsVisible(const std::string &layerName) const
{
    return hiddenLayers_.count(layerName)==0;
}


std::shared_ptr<ConstrainedSketchEntity>
IQVTKConstrainedSketchEditor::selectedItemUnderCursor() const
{
    if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
    {
        if (selact->somethingSelected()
            && selact->currentSelection().size()==1)
        {
            return selact->currentSelection().begin()->lock();
        }
        else if (selact->hasSelectionCandidate())
        {
            return selact->currentSelectionCandidate().lock();
        }
        else if (selact->hasPreviewedItem())
        {
            return selact->previewedItem().lock();
        }
    }
    return nullptr;
}

bool IQVTKConstrainedSketchEditor::onDoubleClick(
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    auto editInPlace = [&](double initValue, std::function<void(double)> setNewValue)
    {
        auto *ed = new QLineEdit(&viewer());
        ed->setGeometry(point.x(), point.y(),
                        ed->size().width(), ed->size().height());

        ed->setText(QString::number(initValue));
        ed->selectAll();

        connect(ed, &QLineEdit::returnPressed, ed,
                [this,ed,setNewValue]()
                {
                    bool ok=false;
                    auto newval=ed->text().toDouble(&ok);
                    setNewValue(newval);
                    solve();
                }
                );
        connect(ed, &QLineEdit::editingFinished,
                ed, &QObject::deleteLater );

        ed->show();
        ed->setFocus(Qt::OtherFocusReason);
    };

    if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
    {
        if (auto selitem = selectedItemUnderCursor())
        {
            if (auto ac = std::dynamic_pointer_cast<FixedAngleConstraint>(selitem))
            {
                editInPlace(
                    ac->targetValue()/SI::deg,
                    [this,ac](double na) {
                        ac->setTargetValue(na*SI::deg);
                    }
                    );
                return true;
            }
            else if (auto dc = std::dynamic_pointer_cast<FixedDistanceConstraint>(selitem))
            {
                editInPlace(
                    dc->targetValue(),
                    std::bind(&FixedDistanceConstraint::setTargetValue, dc, std::placeholders::_1)
                    );
                return true;
            }
            else
            {
                std::set<insight::cad::ConstrainedSketchEntityPtr> tobeadded;
                auto con=sketch().findConnected(selitem);
                std::copy(con.begin(), con.end(),
                          std::inserter(tobeadded, tobeadded.begin()));
                for (auto& tba: tobeadded)
                {
                    selact->externallySelect(tba);
                }
                return true;
            }
        }
    }
    return false;
}




bool IQVTKConstrainedSketchEditor::onKeyRelease ( Qt::KeyboardModifiers modifiers, int key )
{
    if (!toFirstChildAction(&InputReceiver::onKeyRelease, modifiers, key)
        && (key == Qt::Key_Delete) )
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
                return true;
            }
        }
    }

    return false;
}




bool IQVTKConstrainedSketchEditor::onMouseDrag
(
   Qt::MouseButtons buttons,
   Qt::KeyboardModifiers curFlags,
   const QPoint point,
   EventType eventType
)
{
    if ( auto selact = runningAction<IQVTKSelectConstrainedSketchEntity>() )
    {
        if ((buttons==Qt::LeftButton))
        {
            if (auto selitem = selectedItemUnderCursor())
            {
                if (auto sp =
                    std::dynamic_pointer_cast<insight::cad::SketchPoint>(
                        selitem ) )
                {
                    arma::mat pip=viewer().pointInPlane3D(
                        (*this)->plane()->plane(), point );

                    arma::mat p2=viewer().pointInPlane2D(
                        (*this)->plane()->plane(), pip );

                    sp->setCoords2D(p2(0), p2(1));

                    (*this)->invalidate();
                    (*this).updateActors();

                    return true;
                }
                else if (auto dc =
                    std::dynamic_pointer_cast<insight::cad::DistanceConstraint>(
                        selitem ) )
                {
                    launchAction(make_viewWidgetAction<IQVTKDragDimensionlineAction>(*this, dc));

                    return true;
                }
                else if (auto ac =
                         std::dynamic_pointer_cast<insight::cad::AngleConstraint>(
                             selitem ) )
                {
                    launchAction(make_viewWidgetAction<IQVTKDragAngleDimensionAction>(*this, ac));

                    return true;
                }
            }
        }
    }

    return ViewWidgetAction<IQVTKCADModel3DViewer>
        ::onMouseDrag(buttons, curFlags, point, eventType);
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

  // remove vanished geometry
  for (const auto& sga: sketchGeometryActors_)
  {
      auto g=sga.first;
      auto i=(*this)->findGeometry(g);
      if (i==ConstrainedSketch::GeometryMap::const_iterator())
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
    if (!(*this)->hasLayer(newLayerName))
        (*this)->addLayer(newLayerName, *entityProperties_);

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

    (*this)->removeLayer(currentLayerName);

    Q_EMIT sketchChanged();
}



// std::string IQVTKConstrainedSketchEditor::latexRepresentation() const
// {
//     return "";
// }

// std::string IQVTKConstrainedSketchEditor::plainTextRepresentation(int indent) const
// {
//     return "";
// }

// std::unique_ptr<Parameter> IQVTKConstrainedSketchEditor::clone() const
// {
//     throw insight::Exception("not implemented");
//     return nullptr;
// }

// void IQVTKConstrainedSketchEditor::readFromNode(const std::string &name, rapidxml::xml_node<> &node, boost::filesystem::path inputfilepath)
// {}


// int IQVTKConstrainedSketchEditor::nChildren() const
// {
//     return (*this)->size();
// }


// std::string IQVTKConstrainedSketchEditor::childParameterName( int i, bool ) const
// {
//     insight::assertion(
//         i>=0 && i<nChildren(),
//         "invalid child parameter index %d (must be in range 0...%d)",
//         i, nChildren() );

//     return str(boost::format("entity_%d")%i);
// }


// Parameter& IQVTKConstrainedSketchEditor::childParameterRef ( int i )
// {
//     insight::assertion(
//         i>=0 && i<nChildren(),
//         "invalid child parameter index %d (must be in range 0...%d)",
//         i, nChildren() );

//     auto g = (*this)->begin();
//     std::advance(g, i);
//     return g->second->parametersRef();
// }


// const Parameter& IQVTKConstrainedSketchEditor::childParameter( int i ) const
// {
//     insight::assertion(
//         i>=0 && i<nChildren(),
//         "invalid child parameter index %d (must be in range 0...%d)",
//         i, nChildren() );

//     auto g = (*this)->begin();
//     std::advance(g, i);
//     return g->second->parameters();
// }

