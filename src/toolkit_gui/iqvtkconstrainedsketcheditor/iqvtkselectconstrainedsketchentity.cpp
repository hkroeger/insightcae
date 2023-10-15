#include "iqvtkselectconstrainedsketchentity.h"

#include "iqvtkconstrainedsketcheditor.h"
#include "parametereditorwidget.h"

#include <QVBoxLayout>




void SketchEntityMultiSelection::showParameterEditor()
{
    auto spe = new QWidget;
    tbi_=editor_.toolBox_->addItem(spe, "Selection properties");
    editor_.toolBox_->setCurrentIndex(tbi_);

    auto lo = new QVBoxLayout;
    spe->setLayout(lo);

    auto tree=new QTreeView;
    lo->addWidget(tree);
    auto editControls = new QWidget;
    lo->addWidget(editControls);
    pe_ = new ParameterEditorWidget(spe, tree, editControls);


    connect(pe_, &ParameterEditorWidget::parameterSetChanged, pe_,
            [this]()
            {
                for (auto& ee: *this)
                {
                    auto e = ee.lock();
                    e->parametersRef().merge(
                        pe_->model()->getParameterSet(),
                        false
                        );
                }
            }
            );
}

void SketchEntityMultiSelection::removeParameterEditor()
{
    if (pe_)
    {
        editor_.toolBox_->removeItem(tbi_);
        pe_=nullptr;
        tbi_=-1;
    }
}




SketchEntityMultiSelection::SketchEntityMultiSelection
    ( IQVTKConstrainedSketchEditor &editor )
: editor_(editor),
  pe_(nullptr),
  tbi_(-1)
{
}




SketchEntityMultiSelection::~SketchEntityMultiSelection()
{
    removeParameterEditor();
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

            if ( size()>0 && commonParameters_.size()>0 )
            {
                if (!pe_) showParameterEditor();
                pe_->clearParameterSet();
                pe_->resetParameterSet(commonParameters_, defaultCommonParameters_);
            }
            else
            {
                removeParameterEditor();
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
        editor.viewer() ),
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
