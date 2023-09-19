#include "iqvtkselectcadentity.h"

#include <QCheckBox>
#include <QToolBar>


std::vector<IQCADModel3DViewer::CADEntity> IQVTKSelectCADEntity::findEntitiesUnderCursor(const QPoint &point) const
{
    std::vector<IQCADModel3DViewer::CADEntity> ret;

    auto aa = viewer().findAllActorsUnderCursorAt(point);
    for (auto& a: aa)
    {
        for (auto& dd: viewer().displayedData_)
        {
            auto i = std::find(
                dd.second.actors_.begin(),
                dd.second.actors_.end(), a);
            if (i!=dd.second.actors_.end())
            {
                ret.push_back(dd.second.ce_);
                continue;
            }
        }
    }
    return ret;
}




IQVTKCADModel3DViewer::HighlightingHandleSet IQVTKSelectCADEntity::highlightEntity(
    IQCADModel3DViewer::CADEntity entity, QColor hicol ) const
{
    IQVTKCADModel3DViewer::HighlightingHandleSet ret;
    for (auto& dd: viewer().displayedData_)
    {
        if (dd.second.ce_==entity)
        {
            for (auto& a: dd.second.actors_)
            {
                ret.insert(viewer().highlightActor(a, hicol));
            }
        }
    }
    return ret;
}




IQVTKSelectCADEntity::IQVTKSelectCADEntity(IQVTKCADModel3DViewer& viewer)
    : IQVTKCADModel3DViewerSelectionLogic(
        []() { return std::make_shared<MultiSelectionContainer>(); },
        viewer )
{
    toolBar_ = this->viewer().addToolBar("Selection");
    auto selt = new QCheckBox("Preview selection");
    selt->setCheckState(hoveringSelectionPreview()?Qt::Checked:Qt::Unchecked);
    connect(selt, &QCheckBox::toggled, selt,
            [this](bool checked)
            {
                toggleHoveringSelectionPreview(checked);
            }
    );
    toolBar_->addWidget(selt);
}




IQVTKSelectCADEntity::~IQVTKSelectCADEntity()
{
    delete toolBar_;
}





std::vector<IQVTKCADModel3DViewer::SubshapeData>
IQVTKSelectSubshape::findEntitiesUnderCursor(const QPoint &point) const
{
    std::vector<IQVTKCADModel3DViewer::SubshapeData> ret;

    auto aa = viewer().findAllActorsUnderCursorAt(point);
    for (auto& a: aa)
    {
        auto i=viewer().currentSubshapeSelection_->find(a);
        if (i!=viewer().currentSubshapeSelection_->end())
        {
            ret.push_back(i->second);
        }
    }
    return ret;
}



IQVTKCADModel3DViewer::HighlightingHandleSet
IQVTKSelectSubshape::highlightEntity(IQVTKCADModel3DViewer::SubshapeData entity, QColor hicol) const
{
    IQVTKCADModel3DViewer::HighlightingHandleSet ret;
    auto i=std::find_if(
        viewer().currentSubshapeSelection_->begin(),
        viewer().currentSubshapeSelection_->end(),
        [&entity](const IQVTKCADModel3DViewer::SubshapeSelection::value_type& ss)
            {
                return ss.second==entity;
            }
        );
    if (i!=viewer().currentSubshapeSelection_->end())
    {
        ret.insert(viewer().highlightActor(i->first, hicol));
    }
    return ret;
}

IQVTKSelectSubshape::IQVTKSelectSubshape(IQVTKCADModel3DViewer &viewer)
    : IQVTKCADModel3DViewerSubshapeSelectionLogic(
        []() { return std::make_shared<MultiSelectionContainer>(); },
        viewer )
{}



IQVTKSelectSubshape::~IQVTKSelectSubshape()
{}
