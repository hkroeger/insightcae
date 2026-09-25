#include "iqvtkselectcadentity.h"
#include "iqfilteredparametersetmodel.h"
#include "iqparametersetmodel.h"

#include "base/elementpath.h"

#include <QCheckBox>
#include <QToolBar>

#include <QVBoxLayout>
#include <QTreeView>

#include <QToolBox>
#include <QDockWidget>

#include <QSortFilterProxyModel>
#include <algorithm>
#include <qnamespace.h>


void CADEntityMultiSelection::showParameterEditor()
{

    editorContainerWidget_ = new QWidget;
    viewer_.addToolBox(
        editorContainerWidget_, "Selection Properties");

    // viewer_.commonToolBox()->setCurrentIndex(tbi);

    auto lo = new QVBoxLayout;
    editorContainerWidget_->setLayout(lo);

    auto tree=new QTreeView;
    lo->addWidget(tree);
    auto editControls = new QWidget;
    lo->addWidget(editControls);

    editorWidget_ = new ParameterEditorWidget(
        editorContainerWidget_, tree, editControls, &viewer_);

}

void CADEntityMultiSelection::removeParameterEditor()
{
    if (editorContainerWidget_)
    {
        delete editorContainerWidget_;
        editorContainerWidget_=nullptr;
        editorWidget_=nullptr;
    }
}




std::vector<std::string> CADEntityMultiSelection::getParamListForFeature(
    const insight::cad::FeaturePtr& feat) const
{
    auto index = viewer_.cadmodel()->modelstepIndexFromValue(feat);
    std::string assocPs =
        index.siblingAtColumn(IQCADItemModel::assocParamPathsCol)
             .data().toString().toStdString();
    std::vector<std::string> result;
    if (!assocPs.empty())
        boost::split(result, assocPs, boost::is_any_of(":"));
    return result;
}




std::vector<CADEntityMultiSelection::TopLevelEntry>
CADEntityMultiSelection::getTopLevelEntries(
    const std::vector<std::string>& assocParamPaths,
    QAbstractItemModel* apsm) const
{
    std::vector<TopLevelEntry> result;
    if (assocParamPaths.empty()) return result;

    auto* psm = parameterSetModel(apsm);

    auto addFromIndex = [&](QModelIndex idx)
    {
        std::string label =
            psm->data(idx, Qt::DisplayRole).toString().toStdString();
        auto path = psm->elementOfIndex(idx)->path();
        result.push_back({label, path});
    };

    for (const auto& path : assocParamPaths)
    {
        insight::ElementPath ep(path);
        if (ep.back()!="*")
        {
            auto idx = psm->indexOfPath(path, IQParameterSetModel::labelCol);
            addFromIndex(idx);
        }
        else
        {
            ep.erase(--ep.end()); // remove "/*"
            auto pidx = psm->indexOfPath(ep, IQParameterSetModel::labelCol);
            for (int r=0; r<psm->rowCount(pidx); ++r)
            {
                addFromIndex(psm->index(r, 0, pidx));
            }
        }
    }
    return result;
}




CADEntityMultiSelection::CADEntityMultiSelection(
    IQVTKCADModel3DViewer& viewer )
  : viewer_(viewer),
    editorContainerWidget_(nullptr),
    editorWidget_(nullptr)
{
}




CADEntityMultiSelection::~CADEntityMultiSelection()
{
    if (!empty()) emit selectionCleared();
    removeParameterEditor();
}




bool CADEntityMultiSelection::computeEntries(
    std::vector<TopLevelEntry>& entries,
    std::map<std::string, std::set<std::string> >& copyMapping ) const
{
    entries.clear();
    copyMapping.clear();

    auto apsm = viewer_.cadmodel()->associatedParameterSetModel();
    if (!apsm || empty()) return false;

    auto psm = parameterSetModel(apsm);

    bool first = true;
    for (auto& e : *this)
    {
        auto featPtr = boost::get<insight::cad::FeaturePtr>(&e);
        if (!featPtr) return false;

        auto featEntries = getTopLevelEntries(getParamListForFeature(*featPtr), apsm);

        if (first)
        {
            entries = featEntries;
            first = false;
        }
        else
        {
            // Build (label -> indices) map for this entity's entries
            std::map<std::string, std::vector<int>> labelToIdx;
            for (int i = 0; i < static_cast<int>(featEntries.size()); ++i)
                labelToIdx[featEntries[i].label].push_back(i);

            std::map<std::string, int> labelUsed;
            std::vector<TopLevelEntry> newEntries;

            for (auto& ce : entries)
            {
                auto it = labelToIdx.find(ce.label);
                if (it == labelToIdx.end()) continue;

                int& usedIdx = labelUsed[ce.label];
                if (usedIdx >= static_cast<int>(it->second.size())) continue;

                // Also require the same parameter type
                try {
                    if (psm->parameterRef(ce.absPath).type() !=
                        psm->parameterRef(featEntries[it->second[usedIdx]].absPath).type())
                    { ++usedIdx; continue; }
                } catch (...) { continue; }

                auto tba=featEntries[it->second[usedIdx++]].absPath;
                if (tba!=ce.absPath)
                {
                    copyMapping[ce.absPath].insert(tba);
                }
                newEntries.push_back(ce);
            }
            entries = newEntries;
        }
    }

    return !entries.empty();
}




void CADEntityMultiSelection::rebuildEditor()
{
    if (copyConnection_) { disconnect(copyConnection_); copyConnection_ = {}; }
    copyingInProgress_ = false;

    if (!computeEntries(currentEntries_, copyMapping_))
    {
        removeParameterEditor();
        return;
    }

    auto apsm = viewer_.cadmodel()->associatedParameterSetModel();
    auto psm = parameterSetModel(apsm);

    std::vector<std::string> filterPaths;
    for (auto& e : currentEntries_)
        filterPaths.push_back(e.absPath);

    if (!editorWidget_) showParameterEditor();

    auto* spm = new IQFilteredParameterSetModel(filterPaths, editorWidget_);
    spm->setSourceModel(apsm);
    editorWidget_->setModel(spm);

    // the previous filtered model is not displayed any more
    if (filteredModel_) filteredModel_->deleteLater();
    filteredModel_ = spm;

    if (!copyMapping_.empty())
    {
        copyConnection_ = connect(
            apsm, &QAbstractItemModel::dataChanged,
            this, [this, apsm, psm](
                const QModelIndex& topLeft,
                const QModelIndex& /*bottomRight*/,
                const QVector<int>& /*roles*/)
            {
                if (copyingInProgress_) return;

                std::string changedPath =
                    apsm->data(topLeft.siblingAtColumn(IQParameterSetModel::stringPathCol))
                        .toString().toStdString();

                for (auto& [P1, pkList] : copyMapping_)
                {
                    bool match =
                        (changedPath == P1) ||
                        (changedPath.size() > P1.size() + 1 &&
                         changedPath.compare(0, P1.size(), P1) == 0 &&
                         changedPath[P1.size()] == '/');
                    if (!match) continue;

                    std::string relPath = (changedPath == P1)
                        ? "" : changedPath.substr(P1.size() + 1);

                    copyingInProgress_ = true;
                    {
                        auto bulkGuard = psm->beginBulkUpdate();
                        for (auto& Pk : pkList)
                        {
                            std::string targetPath = relPath.empty() ? Pk : Pk + "/" + relPath;
                            try {
                                psm->parameterRef(targetPath)
                                   .assignFrom(psm->parameterRef(changedPath));
                                psm->notifyElementChange(psm->parameterRef(targetPath));
                            } catch (...) {}
                        }
                    }  // BulkUpdateGuard dtor: one undo state stored, bulkUpdateFinished emitted
                    copyingInProgress_ = false;
                    break;
                }
            });
    }
}




void CADEntityMultiSelection::refreshEditorIfNeeded()
{
    refreshPending_ = false;

    std::vector<TopLevelEntry> entries;
    std::map<std::string, std::set<std::string> > copyMapping;
    bool ok = computeEntries(entries, copyMapping);

    auto samePaths = [](const std::vector<TopLevelEntry>& a,
                        const std::vector<TopLevelEntry>& b)
    {
        return std::equal(
            a.begin(), a.end(), b.begin(), b.end(),
            [](const TopLevelEntry& x, const TopLevelEntry& y)
            { return x.absPath==y.absPath; } );
    };

    if ( ok && editorWidget_
         && samePaths(entries, currentEntries_)
         && copyMapping==copyMapping_ )
    {
        return; // nothing changed, keep the editor as it is
    }

    rebuildEditor();
}




void CADEntityMultiSelection::replace(
    IQCADModel3DViewer::CADEntity oldEntity,
    IQCADModel3DViewer::CADEntity newEntity )
{
    if (count(oldEntity) < 1) return;

    // exchange silently: the entity represents the same model item
    std::set<IQCADModel3DViewer::CADEntity>::erase(oldEntity);
    std::set<IQCADModel3DViewer::CADEntity>::insert(newEntity);

    // the associated parameter paths of the new entity are updated
    // in the model only after the entity exchange has been signalled:
    // check the editor later
    if (!refreshPending_)
    {
        refreshPending_ = true;
        QMetaObject::invokeMethod(
            this, &CADEntityMultiSelection::refreshEditorIfNeeded,
            Qt::QueuedConnection );
    }
}




void CADEntityMultiSelection::insert(
    IQCADModel3DViewer::CADEntity entity)
{
    if (count(entity) >= 1) // don't add multiple times
        return;
    std::set<IQCADModel3DViewer::CADEntity>::insert(entity);
    emit entityInserted(entity);
    rebuildEditor();
}




void CADEntityMultiSelection::erase(IQCADModel3DViewer::CADEntity entity)
{
    std::set<IQCADModel3DViewer::CADEntity>::erase(entity);
    emit entityErased(entity);
    rebuildEditor();
}


void CADEntityMultiSelection::insertMany(
    const std::vector<IQCADModel3DViewer::CADEntity>& entities)
{
    for (const auto& entity : entities)
    {
        if (count(entity) >= 1)
            continue;
        std::set<IQCADModel3DViewer::CADEntity>::insert(entity);
        emit entityInserted(entity);
    }
    rebuildEditor();
}


void CADEntityMultiSelection::eraseMany(
    const std::vector<IQCADModel3DViewer::CADEntity>& entities)
{
    for (const auto& entity : entities)
    {
        if (count(entity) < 1)
            continue;
        std::set<IQCADModel3DViewer::CADEntity>::erase(entity);
        emit entityErased(entity);
    }
    rebuildEditor();
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
        [this, &viewer]() {
            auto ms = std::make_shared<CADEntityMultiSelection>(viewer);
            connect(ms.get(), &CADEntityMultiSelection::entityInserted,
                    this, &IQVTKSelectCADEntity::onEntityInserted,
                    Qt::DirectConnection);
            connect(ms.get(), &CADEntityMultiSelection::entityErased,
                    this, &IQVTKSelectCADEntity::onEntityErased,
                    Qt::DirectConnection);
            connect(ms.get(), &CADEntityMultiSelection::selectionCleared,
                    this, &IQVTKSelectCADEntity::onSelectionCleared,
                    Qt::DirectConnection);
            return ms;
        },
        viewer,
        false // captureAllInput
    )
{}




IQVTKSelectCADEntity::~IQVTKSelectCADEntity()
{
    delete toolBar_;
}

QString IQVTKSelectCADEntity::description() const
{
    return "Select CAD entity";
}

bool IQVTKSelectCADEntity::onMouseClick  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    if (btn==Qt::RightButton)
    {
        viewer().contextMenuClick(
            viewer().mapToGlobal(point) );
        return true;
    }
    return IQVTKCADModel3DViewerSelectionLogic
        ::onMouseClick(btn, nFlags, point);
}

void IQVTKSelectCADEntity::onEntityInserted(IQCADModel3DViewer::CADEntity entity)
{
    for (auto& [midx, dd] : viewer().displayedData_)
        if (dd.ce_ == entity) { viewer().viewerEntitySelected(midx); break; }
}

void IQVTKSelectCADEntity::onEntityErased(IQCADModel3DViewer::CADEntity entity)
{
    for (auto& [midx, dd] : viewer().displayedData_)
        if (dd.ce_ == entity) { viewer().viewerEntityDeselected(midx); break; }
}

void IQVTKSelectCADEntity::onSelectionCleared()
{
    viewer().viewerSelectionCleared();
}



bool IQVTKSelectCADEntity::onKeyPress(
    Qt::KeyboardModifiers modifiers, int key)
{
    if (key == Qt::Key_A && (modifiers & Qt::ControlModifier))
    {
        for (auto& dd : viewer().displayedData_)
        {
            bool visible = false;
            for (auto& act : dd.second.actors_)
            {
                if (act->GetVisibility())
                {
                    visible = true;
                    break;
                }
            }
            if (visible)
                externallySelect(dd.second.ce_);
        }
        return true;
    }
    return IQVTKCADModel3DViewerSelectionLogic::onKeyPress(modifiers, key);
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
        viewer,
        false // captureAllInput
      )
{
    toggleHoveringSelectionPreview(true);
}



IQVTKSelectSubshape::~IQVTKSelectSubshape()
{}

QString IQVTKSelectSubshape::description() const
{
    return "Select sub shape";
}


void IQVTKSelectSubshape::start()
{}
