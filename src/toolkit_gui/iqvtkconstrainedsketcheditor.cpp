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





IQVTKFixedPoint::IQVTKFixedPoint(
        insight::cad::SketchPointPtr p  )
    : p_(p)
{
    auto c=p->coords2D();
    changeDefaultParameters(
                insight::ParameterSet({
                                 {"x", new insight::DoubleParameter(c(0), "x location")},
                                 {"y", new insight::DoubleParameter(c(1), "y location")}
                             })
                );
}

std::vector<vtkSmartPointer<vtkProp> > IQVTKFixedPoint::createActor() const
{
//    auto sph = vtkSmartPointer<vtkSphereSource>::New();
//    sph->SetCenter( p_->value().memptr() );
//#warning need a sketch order of scene size
//    sph->SetRadius(0.025);

//    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
//    //mapper->SetInputData(arr);
//    mapper->SetInputConnection(sph->GetOutputPort());
//    mapper->Update();

//    auto act = vtkSmartPointer<vtkActor>::New();
//    act->SetMapper( mapper );

//    return {act};

//    auto text = vtkSmartPointer<vtkVectorText>::New();
//    text->SetText("X");
//    auto mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
//    mapper->SetInputConnection(text->GetOutputPort());
//    auto follower=vtkSmartPointer<vtkFollower>::New();
//    follower->SetMapper(mapper);
//    follower->SetScale(0.1);
//    follower->AddPosition( p_->value().memptr() );
//    return {follower};


    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("X");
    caption->SetAttachmentPoint(p_->value().memptr());
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

    return {caption};
}

int IQVTKFixedPoint::nConstraints() const
{
    return 1;
}

double IQVTKFixedPoint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
                iConstraint==0,
                "invalid constraint id: %d", iConstraint );
    auto p = p_->coords2D();
    double x=parameters().getDouble("x");
    double y=parameters().getDouble("y");
    return pow(p(0)-x,2), pow(p(1)-y, 2);
}

void IQVTKFixedPoint::scaleSketch(double scaleFactor)
{
    parametersRef().get<insight::DoubleParameter>("x")() *= scaleFactor;
    parametersRef().get<insight::DoubleParameter>("y")() *= scaleFactor;
}

void IQVTKFixedPoint::generateScriptCommand(insight::cad::ConstrainedSketchScriptBuffer &script, const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{

}






IQVTKHorizontalConstraint::IQVTKHorizontalConstraint(
    std::shared_ptr<insight::cad::Line const> line )
    : line_(line)
{}


std::vector<vtkSmartPointer<vtkProp> > IQVTKHorizontalConstraint::createActor() const
{
//    auto text = vtkSmartPointer<vtkVectorText>::New();
//    text->SetText("H");
//    auto mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
//    mapper->SetInputConnection(text->GetOutputPort());
//    auto follower=vtkSmartPointer<vtkFollower>::New();
//    follower->SetMapper(mapper);
//    follower->AddPosition( arma::mat(0.5*
//       (line_->getDatumPoint("p0")+line_->getDatumPoint("p1")))
//                              .memptr()
//    );
//    return {follower};

    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("H");
    caption->SetAttachmentPoint(
        arma::mat(0.5*
                  (line_->getDatumPoint("p0")+line_->getDatumPoint("p1")))
            .memptr());
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

    return {caption};
}

int IQVTKHorizontalConstraint::nConstraints() const
{
    return 1;
}

double IQVTKHorizontalConstraint::getConstraintError(unsigned int iConstraint) const
{
    arma::mat ex = insight::normalized(
        line_->getDatumVectors().at("ex") );
    return 1.-fabs(arma::dot(insight::vec3(1,0,0), ex));
}

void IQVTKHorizontalConstraint::scaleSketch(double scaleFactor)
{}

void IQVTKHorizontalConstraint::generateScriptCommand(insight::cad::ConstrainedSketchScriptBuffer &script, const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{

}







IQVTKVerticalConstraint::IQVTKVerticalConstraint(
    std::shared_ptr<insight::cad::Line const> line )
    : line_(line)
{}

std::vector<vtkSmartPointer<vtkProp> > IQVTKVerticalConstraint::createActor() const
{
//    auto text = vtkSmartPointer<vtkVectorText>::New();
//    text->SetText("V");
//    auto mapper=vtkSmartPointer<vtkPolyDataMapper>::New();
//    mapper->SetInputConnection(text->GetOutputPort());
//    auto follower=vtkSmartPointer<vtkFollower>::New();
//    follower->SetMapper(mapper);
//    follower->AddPosition( arma::mat(0.5*
//                                    (line_->getDatumPoint("p0")+line_->getDatumPoint("p1")))
//                              .memptr()
//                          );
//    return {follower};

    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("V");
    caption->SetAttachmentPoint(
        arma::mat(0.5*
                  (line_->getDatumPoint("p0")+line_->getDatumPoint("p1")))
            .memptr());
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

    return {caption};

}
int IQVTKVerticalConstraint::nConstraints() const
{
    return 1;
}

double IQVTKVerticalConstraint::getConstraintError(unsigned int iConstraint) const
{
    arma::mat ex = insight::normalized(
        line_->getDatumVectors().at("ex") );
    return 1.-fabs(arma::dot(insight::vec3(0,1,0), ex));
}

void IQVTKVerticalConstraint::scaleSketch(double scaleFactor)
{}

void IQVTKVerticalConstraint::generateScriptCommand(insight::cad::ConstrainedSketchScriptBuffer &script, const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{

}






IQVTKPointOnCurveConstraint::IQVTKPointOnCurveConstraint(
    std::shared_ptr<insight::cad::SketchPoint const> p,
    std::shared_ptr<insight::cad::Feature const> curve )
  : p_(p),
    curve_(curve)
{}

std::vector<vtkSmartPointer<vtkProp> > IQVTKPointOnCurveConstraint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("F");
    caption->SetAttachmentPoint(
        arma::mat(p_->value()).memptr()
        );
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

    return {caption};
}

int IQVTKPointOnCurveConstraint::nConstraints() const
{
    return 1;
}

double IQVTKPointOnCurveConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
        iConstraint==0,
        "invalid constraint index");

    return curve_->minDist(p_->value());
}

void IQVTKPointOnCurveConstraint::scaleSketch(double scaleFactor)
{
}

void IQVTKPointOnCurveConstraint::generateScriptCommand(insight::cad::ConstrainedSketchScriptBuffer &script, const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{

}





void IQVTKConstrainedSketchEditor::add(insight::cad::ConstrainedSketchEntityPtr sg)
{
    auto setapp = setActorAppearance_;
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
        setapp=[](const insight::ParameterSet& p, vtkProperty* prop)
            {
                prop->SetColor(0.7, 0.3, 0.3);
                prop->SetLineWidth(0.5);
            };
    }
    else if (auto soe = std::dynamic_pointer_cast<IQVTKConstrainedSketchEntity>(sg)) // e.g. constrains etc.
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
            follower->SetCamera(viewer().renderer()->GetActiveCamera());
        }
        viewer().renderer()->AddActor(a);
        as.insert(a);
    }
    sketchGeometryActors_.emplace(sg, as);
}




void IQVTKConstrainedSketchEditor::remove(insight::cad::ConstrainedSketchEntityPtr sg)
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


void IQVTKConstrainedSketchEditor::SketchEntitySelection::highlight(
        std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity )
{
    auto i=std::find_if(
                editor_.sketchGeometryActors_.begin(),
                editor_.sketchGeometryActors_.end(),
                [&entity]( const decltype(editor_.sketchGeometryActors_)::value_type& p2 )
        {
            return !entity.owner_before(p2.first) && !p2.first.owner_before(entity);
        }
    );

    editor_.viewer().actorHighlight_.reset();
    if (i!=editor_.sketchGeometryActors_.end())
    {
        mapped_type restoreProps;
        ActorSet actors = i->second;
        for(auto &a: actors)
        {
            if (auto aa = vtkActor::SafeDownCast(a))
            {
                decltype(restoreProps)::mapped_type prop;
                aa->GetProperty()->GetColor(prop.oldColor);
                aa->GetProperty()->SetColor(0,0,1);
                restoreProps[aa]=prop;
            }
        }
        insert({entity, restoreProps});
    }
}

void IQVTKConstrainedSketchEditor::SketchEntitySelection::unhighlight(
        std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity)
{
    auto i = find(entity);
    if (i!=end())
    {
        auto restoreProp=i->second;
        for (auto& rp: restoreProp)
        {
            if (auto aa = vtkActor::SafeDownCast(rp.first))
            {
                aa->GetProperty()->SetColor(rp.second.oldColor);
            }
        }
        erase(i);
    }
}

IQVTKConstrainedSketchEditor::SketchEntitySelection::SketchEntitySelection
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


IQVTKConstrainedSketchEditor::SketchEntitySelection::~SketchEntitySelection()
{
    editor_.toolBox_->removeItem(tbi_);
    for (iterator i=begin(); i!=end(); i=begin())
    {
        if (!i->first.expired())
        {
            unhighlight(i->first);
        }
    }
}


void IQVTKConstrainedSketchEditor::SketchEntitySelection::addAndHighlight(
        insight::cad::ConstrainedSketchEntityPtr entity)
{
    if (!isInSelection(entity)) // don't add multiple times
    {
        highlight(entity);

        if (size()==1)
        {
            commonParameters_=
                entity->parameters();
            defaultCommonParameters_=
                entity->defaultParameters();
        }
        else if (size()>1)
        {
            commonParameters_=
                commonParameters_.intersection(entity->parameters());
            defaultCommonParameters_=
                defaultCommonParameters_.intersection(entity->defaultParameters());
        }

        if (size()>0)
        {
            pe_->clearParameterSet();
            pe_->resetParameterSet(commonParameters_, defaultCommonParameters_);
        }
    }
}

bool IQVTKConstrainedSketchEditor::SketchEntitySelection::isInSelection(
        const insight::cad::ConstrainedSketchEntityPtr &entity)
{
    return find(entity)!=end();
}



void IQVTKConstrainedSketchEditor::drawLine()
{
    auto dl = std::make_shared<IQVTKCADModel3DViewerDrawLine>(viewer(), *this);

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::updateActors,
            this, &IQVTKConstrainedSketchEditor::updateActors );

    connect(dl.get(), &IQVTKCADModel3DViewerDrawLine::endPointSelected, dl.get(),

            [this]( IQVTKCADModel3DViewerDrawLine::CandidatePoint* addPoint,
                    insight::cad::SketchPointPtr previousPoint )
            {
                if (addPoint->onFeature)
                {
                    (*this)->geometry().insert(
                        std::make_shared
                        <IQVTKPointOnCurveConstraint>( addPoint->sketchPoint, addPoint->onFeature ) ); // fix to curve

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
                        std::make_shared
                        <IQVTKFixedPoint>( addPoint->sketchPoint ) ); // fix first point
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
    (*this)->resolveConstraints();
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
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_drawline.svg"), "Line",
                        this, &IQVTKConstrainedSketchEditor::drawLine);
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_solve.svg"), "Solve",
                        this, &IQVTKConstrainedSketchEditor::solve);
    toolBar_->addAction(QPixmap(":/icons/icon_sketch_finish.svg"), "Finish",
                        this, &IQVTKConstrainedSketchEditor::finished);

    toolBar_->addAction("H",
    [&]()
    {
        if (currentSelection_)
        {
            for (auto sele: *currentSelection_)
            {
                auto sg = sele.first.lock();
                if (auto l = std::dynamic_pointer_cast<insight::cad::Line>(sg))
                {
                    auto lc = std::make_shared<IQVTKHorizontalConstraint>( l );
                    (*this)->geometry().insert(lc);
                    (*this)->invalidate();
                    this->updateActors();
                }
            }
        }
    });
    toolBar_->addAction("V",
    [&]()
    {
        if (currentSelection_)
        {
            for (auto sele: *currentSelection_)
            {
                auto sg = sele.first.lock();
                if (auto l = std::dynamic_pointer_cast<insight::cad::Line>(sg))
                {
                    auto lc = std::make_shared<IQVTKVerticalConstraint>( l );
                    (*this)->geometry().insert(lc);
                    (*this)->invalidate();
                    this->updateActors();
                }
            }
        }
    });

    toolBar_->addAction("F",
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
                    auto lc = std::make_shared<IQVTKPointOnCurveConstraint>( pt, curve );
                    (*this)->geometry().insert(lc);
                    (*this)->invalidate();
                    this->updateActors();
                }
            }
        }
    });

    toolBar_->addAction("Scale",
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
                        });
    toolBar_->show();


    updateActors();

    // this editor is its own property widget

    toolBoxWidget_ = new QDockWidget("Properties", this);
    this->viewer().addDockWidget(Qt::RightDockWidgetArea, toolBoxWidget_);
    toolBox_ = new QToolBox(toolBoxWidget_);
    toolBoxWidget_->setWidget(toolBox_);

    auto l = new QFormLayout;

    auto  toledit = new QLineEdit;
    toledit->setValidator(new QDoubleValidator);
    toledit->setText(QString::number((*this)->solverTolerance()));
    connect(toledit, &QLineEdit::textChanged, this,
            [this](const QString& txt)
            {
                double newtol = txt.toDouble();
                if ( fabs(newtol-(*this)->solverTolerance()) > insight::SMALL)
                {
                    (*this)->setSolverTolerance(newtol);
                    solve();
                }
            }
    );

    l->addRow("Solver tolerance", toledit);
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


bool IQVTKConstrainedSketchEditor::onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    bool ret=false;

    if (!ret && currentAction_)
        ret=currentAction_->onLeftButtonDown( nFlags, point );

    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();

    if (!ret)
    {
        if (auto act =
                viewer().findActorUnderCursorAt(point))
        {
            if (auto sg =
                    this->findSketchElementOfActor(act))
            {
                if (!currentSelection_)
                {
                    currentSelection_ =
                            std::make_shared<SketchEntitySelection>(
                                *this);
                }
                currentSelection_->addAndHighlight(sg);
                ret=true;
            }
            else
            {
                currentSelection_.reset();
                ret=true;

            }
        }
        else
        {
            currentSelection_.reset();
            ret=true;
        }
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
    return ret;
}

bool IQVTKConstrainedSketchEditor::onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point )
{
    bool ret=false;
    if (currentAction_)
        ret=currentAction_->onLeftButtonUp( nFlags, point );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
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

bool IQVTKConstrainedSketchEditor::onKeyPress ( Qt::KeyboardModifiers modifiers, int key )
{
    bool ret=false;
    if (currentAction_)
        ret=currentAction_->onKeyPress( modifiers, key );
    if (currentAction_ && currentAction_->finished())
        currentAction_.reset();
    return ret;
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
                auto gptr=td.lock();
                remove(gptr);
                (*this)->geometry().erase(gptr);
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

  viewer().scheduleRedraw();
}
