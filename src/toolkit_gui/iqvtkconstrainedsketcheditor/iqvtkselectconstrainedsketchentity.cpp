#include "iqvtkselectconstrainedsketchentity.h"

#include "base/cppextensions.h"
#include "iqfilteredparametersetmodel.h"
#include "iqvtkconstrainedsketcheditor.h"
#include "parametereditorwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <qnamespace.h>

#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerdrawrectangle.h"

void SketchEntityMultiSelection::showPropertiesEditor(bool includeParameterEditor)
{
    pew_ = new QWidget;
    editor_.viewer().addToolBox(pew_, "Sketch entity properties");

    auto lo = new QVBoxLayout;
    pew_->setLayout(lo);


    auto l=new QHBoxLayout;
    l->addWidget(new QLabel("Layer"));
    auto *layEd=new QComboBox;
    layEd->setEditable(true);
    
    auto ln = editor_->layerNames();
    for (auto& l: ln)
    {
        layEd->addItem(QString::fromStdString(l));
    }

    l->addWidget(layEd);
    lo->addLayout(l);

    std::unique_ptr<std::string> commonLayerName;
    for (auto& ee: *this)
    {
        auto e = ee.lock();
        if (!commonLayerName)
            commonLayerName.reset(
                new std::string(e->layerName()));
        else
        {
            if (*commonLayerName!=e->layerName())
                *commonLayerName="";
        }
    }
    if (commonLayerName)
        layEd->setEditText(
            QString::fromStdString(*commonLayerName));

    auto changeLayer =
            [this,layEd]()
            {
                auto newLayerName = layEd->currentText().toStdString();
                bool anythingDone=false;
                for (auto& ee: *this)
                {
                    auto e = ee.lock();
                    if (!(*editor_).hasLayer(newLayerName))
                    {
                        (*editor_).addLayer(newLayerName, *editor_.entityProperties_);
                        anythingDone=true;
                    }
                    if (e->layerName()!=newLayerName)
                    {
                        e->setLayerName(newLayerName);
                        anythingDone=true;
                    }
                }
                if (anythingDone)
                {
                    Q_EMIT editor_.sketchChanged();
                }
            };

    connect(layEd->lineEdit(), &QLineEdit::editingFinished, layEd,
            changeLayer);
    connect(layEd, QOverload<int>::of(&QComboBox::currentIndexChanged), layEd,
            changeLayer);

    if (includeParameterEditor)
    {
        auto tree=new QTreeView;
        lo->addWidget(tree);
        auto editControls = new QWidget;
        lo->addWidget(editControls);

        pe_ = new ParameterEditorWidget(pew_, tree, editControls, &editor_.viewer());
        connect(pe_, &ParameterEditorWidget::parameterSetChanged, pe_,
                [this]()
                {
                    for (auto& ee: *this)
                    {
                        auto e = ee.lock();
                        e->parametersRef().merge(
                            getParameterSet(pe_->model())
                            );
                    }
                }
                );
    }
}

void SketchEntityMultiSelection::removePropertiesEditor()
{
    if (pe_)
    {
        delete pe_;
        pe_=nullptr;
    }
    if (pew_)
    {
        delete pew_;
        pew_=nullptr;
    }
}




SketchEntityMultiSelection::SketchEntityMultiSelection
    ( IQVTKConstrainedSketchEditor &editor )
: editor_(editor),
  pew_(nullptr),
  pe_(nullptr)
{
}




SketchEntityMultiSelection::~SketchEntityMultiSelection()
{
    removePropertiesEditor();
}




void SketchEntityMultiSelection::insert(
    std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity)
{
    if (count(entity)<1) // don't add multiple times
    {

        auto i = editor_.sketchGeometryActors_.find(
            entity.lock() );

        if (i!=editor_.sketchGeometryActors_.end())
        {

            SketchEntityMultiSelectionSet::insert(entity);

            auto lentity=entity.lock();


            if (size()==1)
            {
                // commonParameters_=
                //     lentity->parameters().cloneSubset();
                // defaultCommonParameters_=
                //     lentity->defaultParameters().cloneSubset();
            }
            else if (size()>1)
            {
                // commonParameters_=std::move(
                //     std::dynamic_unique_ptr_cast<insight::ParameterSet>(
                //     commonParameters_->intersection(
                //     lentity->parameters())));

                // defaultCommonParameters_=std::move(
                //     std::dynamic_unique_ptr_cast<insight::ParameterSet>(
                //     defaultCommonParameters_->intersection(
                //     lentity->defaultParameters())));
            }

            if ( size()>0 )
            {
                if (!pew_)
                    showPropertiesEditor(
                        size()>0 );

                if (auto *psm = editor_.presentation_->setupSketchEntityParameterSetModel(*lentity))
                {
                    pe_->setModel(psm);
                }
            }
            else
            {
                removePropertiesEditor();
            }
        }
    }
}




void SketchEntityMultiSelection::erase(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity)
{
    SketchEntityMultiSelectionSet::erase(entity);
}




std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> >
IQVTKSelectConstrainedSketchEntity::findEntitiesUnderCursor(
    const QPoint& point) const
{
    std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > ret;
    auto aa = viewer().findAllActorsUnderCursorAt(point);
    for (int i=0; i<2; ++i)
    {
        for (auto& a: aa)
        {
            if (auto sg = editor_.findSketchElementOfActor(a))
            {
                // sort points to beginning of list
                if (
                    (i==0 && std::dynamic_pointer_cast<insight::cad::SketchPoint>(sg))
                    ||
                    (i==1 && !std::dynamic_pointer_cast<insight::cad::SketchPoint>(sg))
                    )
                {
                    if (std::find_if(ret.begin(), ret.end(),
                        [&sg](const decltype(ret)::value_type& i){return i.lock()==sg;})
                        == ret.end())
                    {
                        ret.push_back(sg);
                    }
                }
            }
        }
    }
    return ret;
}




IQVTKCADModel3DViewer::HighlightingHandleSet
IQVTKSelectConstrainedSketchEntity::highlightEntity(
    std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity,
    QColor hicol
    ) const
{
    auto lentity=entity.lock();
    auto sg = editor().sketchGeometryActors_.at(lentity);
    std::set<vtkProp*> props;
    std::copy(
        sg.begin(), sg.end(),
        std::inserter(props, props.begin())
        );
    return viewer().highlightActors(props, hicol);
}




IQVTKSelectConstrainedSketchEntity::IQVTKSelectConstrainedSketchEntity(
    IQVTKConstrainedSketchEditor &editor,
    bool allowBoxSelection )
    :   IQVTKConstrainedSketchEditorSelectionLogic(
        [&editor]()
        { return std::make_shared<SketchEntityMultiSelection>(editor); },
        editor.viewer(),
        false // captureAllInput
        ),
    editor_(editor),
    allowBoxSelection_(allowBoxSelection)
{
    toggleHoveringSelectionPreview(true);
}



IQVTKSelectConstrainedSketchEntity::~IQVTKSelectConstrainedSketchEntity()
{}



void IQVTKSelectConstrainedSketchEntity::start()
{}



bool IQVTKSelectConstrainedSketchEntity::onMouseClick(
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point)
{
    if (!this->hasChildReceivers())
    {
        bool ret=IQVTKConstrainedSketchEditorSelectionLogic
            ::onMouseClick(btn, nFlags, point);

        if (!ret
            && (btn==Qt::LeftButton) && (nFlags==Qt::ShiftModifier)
            && allowBoxSelection_ )
        {
            auto dl = make_viewWidgetAction<IQVTKCADModel3DViewerDrawRectangle>(
                editor(), false, false );

            dl->previewUpdated.connect(
                [this](const arma::mat& p1_3d, const arma::mat& p2_3d)
                {
                    auto p1=(*editor_).p3Dto2D(p1_3d);
                    auto p2=(*editor_).p3Dto2D(p2_3d);

                    auto selectedEntities =
                        (*editor_).entitiesInsideRect(
                            p1[0], p1[1], p2[0], p2[1] );

                    setSelectionTo(selectedEntities);
                }
                );

            auto& dlRef=*dl;
            launchAction(std::move(dl));
            return dlRef.onMouseClick(btn, nFlags, point);
        }
        return ret;
    }
    else
        return IQVTKConstrainedSketchEditorSelectionLogic
            ::onMouseClick(btn, nFlags, point);
}



IQVTKConstrainedSketchEditor &IQVTKSelectConstrainedSketchEntity::editor() const
{
    return editor_;
}



insight::cad::ConstrainedSketch &IQVTKSelectConstrainedSketchEntity::sketch() const
{
    return *editor_;
}
