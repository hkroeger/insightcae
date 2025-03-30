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
                for (auto& ee: *this)
                {
                    auto e = ee.lock();
                    if (!(*editor_).hasLayer(newLayerName))
                    {
                        (*editor_).addLayer(newLayerName, *editor_.entityProperties_);
                    }
                    e->setLayerName(newLayerName);
                }
                editor_.sketchChanged();
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
    for (auto& a: aa)
    {
        if (auto sg =
            editor_.findSketchElementOfActor(a))
        {
            ret.push_back(sg);
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
    IQVTKConstrainedSketchEditor &editor )
    :   IQVTKConstrainedSketchEditorSelectionLogic(
        [&editor]()
        { return std::make_shared<SketchEntityMultiSelection>(editor); },
        editor.viewer(),
        false // captureAllInput
        ),
    editor_(editor)
{
    toggleHoveringSelectionPreview(true);
}


IQVTKSelectConstrainedSketchEntity::~IQVTKSelectConstrainedSketchEntity()
{}

void IQVTKSelectConstrainedSketchEntity::start()
{}


IQVTKConstrainedSketchEditor &IQVTKSelectConstrainedSketchEntity::editor() const
{
    return editor_;
}

insight::cad::ConstrainedSketch &IQVTKSelectConstrainedSketchEntity::sketch() const
{
    return *editor_;
}
