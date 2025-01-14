#include "iqvtkselectcadentity.h"
#include "iqfilteredparametersetmodel.h"

#include <QCheckBox>
#include <QToolBar>

#include <QVBoxLayout>
#include <QTreeView>

#include <QToolBox>
#include <QDockWidget>

#include <QSortFilterProxyModel>


void CADEntityMultiSelection::showParameterEditor()
{

    pew_ = new QWidget;
    auto tbi = viewer_.commonToolBox()->addItem(pew_, "Selection Properties");
    viewer_.commonToolBox()->setCurrentIndex(tbi);

    auto lo = new QVBoxLayout;
    pew_->setLayout(lo);

    auto tree=new QTreeView;
    lo->addWidget(tree);
    auto editControls = new QWidget;
    lo->addWidget(editControls);
    pe_ = new ParameterEditorWidget(pew_, tree, editControls, &viewer_);

}

void CADEntityMultiSelection::removeParameterEditor()
{
    if (pew_)
    {
        delete pew_;
        pew_=nullptr;
        pe_=nullptr;
    }
}




CADEntityMultiSelection::CADEntityMultiSelection(
    IQVTKCADModel3DViewer& viewer )
  : viewer_(viewer),
    pew_(nullptr),
    pe_(nullptr)
{
}




CADEntityMultiSelection::~CADEntityMultiSelection()
{
    removeParameterEditor();
}




void CADEntityMultiSelection::insert(
    IQCADModel3DViewer::CADEntity entity)
{
    if (count(entity)<1) // don't add multiple times
    {
        std::set<IQCADModel3DViewer::CADEntity>::insert(entity);

        auto featPtr = boost::get<insight::cad::FeaturePtr>(&entity);
        IQFilteredParameterSetModel *spm=nullptr;

        if ( (size() == 1) && featPtr )
        {
            if (auto apsm = viewer_.cadmodel()->associatedParameterSetModel())
            {
                auto index = viewer_.cadmodel()->modelstepIndexFromValue( *featPtr );
                std::vector<std::string> paramList;
                std::string assocPs=
                    index.siblingAtColumn(IQCADItemModel::assocParamPathsCol)
                                          .data()
                                          .toString()
                                          .toStdString();
                boost::split(
                    paramList, assocPs,
                    boost::is_any_of(":") );

                if (paramList.size())
                {
                    if (!pe_) showParameterEditor();
                    spm=new IQFilteredParameterSetModel(paramList, pe_);
                    spm->setSourceModel(apsm);
                    pe_->setModel(spm);
                }
            }
        }

        if (!spm && pe_)
        {
            removeParameterEditor();
        }
    }
}




void CADEntityMultiSelection::erase(IQCADModel3DViewer::CADEntity entity)
{
    std::set<IQCADModel3DViewer::CADEntity>::erase(entity);
}




std::vector<IQCADModel3DViewer::CADEntity>
IQVTKSelectCADEntity::findEntitiesUnderCursor(const QPoint &point) const
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
        [&viewer]()
        { return std::make_shared<CADEntityMultiSelection>(viewer); },
        viewer )
{}




IQVTKSelectCADEntity::~IQVTKSelectCADEntity()
{
    delete toolBar_;
}

void IQVTKSelectCADEntity::start()
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


void IQVTKSelectSubshape::start()
{}
