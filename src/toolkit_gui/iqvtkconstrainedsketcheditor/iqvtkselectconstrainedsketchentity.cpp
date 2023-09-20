#include "iqvtkselectconstrainedsketchentity.h"

#include "iqvtkconstrainedsketcheditor.h"
#include "parametereditorwidget.h"

#include <QVBoxLayout>

SketchEntityMultiSelection::SketchEntityMultiSelection
    ( IQVTKConstrainedSketchEditor &editor )
    : editor_(editor)
{
    auto spe = new QWidget;
    auto lo = new QVBoxLayout;
    spe->setLayout(lo);
    tbi_=editor_.toolBox_->addItem(spe, "Selection properties");

    auto tree=new QTreeView;
    lo->addWidget(tree);
    auto editControls = new QWidget;
    lo->addWidget(editControls);
    pe_ = new ParameterEditorWidget(spe, tree, editControls);

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
}




void SketchEntityMultiSelection::insert(
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
            SketchEntityMultiSelectionMap::insert({entity, hl});

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




void SketchEntityMultiSelection::erase(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity)
{
    SketchEntityMultiSelectionMap::erase(entity);
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


IQVTKConstrainedSketchEditor &IQVTKSelectConstrainedSketchEntity::editor() const
{
    return editor_;
}

insight::cad::ConstrainedSketch &IQVTKSelectConstrainedSketchEntity::sketch() const
{
    return *editor_;
}
