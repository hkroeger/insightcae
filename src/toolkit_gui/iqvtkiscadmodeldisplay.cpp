#include "iqvtkiscadmodeldisplay.h"

#include <QItemSelectionModel>
#include <QHeaderView>

#include <vector>

#include "iqgroupingitemmodel.h"


static void collectGroupChildren(
    IQGroupingItemModel* model,
    const QModelIndex& parent,
    QItemSelectionModel* treeSelModel,
    QItemSelectionModel::SelectionFlags flags,
    std::vector<QModelIndex>& collected)
{
    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        auto child = model->index(row, 0, parent);
        auto srcIdx = model->mapToSource(child);
        if (srcIdx.isValid())
        {
            treeSelModel->select(child, flags | QItemSelectionModel::Rows);
            collected.push_back(srcIdx);
        }
        else
        {
            collectGroupChildren(model, child, treeSelModel, flags, collected);
        }
    }
}


IQVTKISCADModelDisplay::IQVTKISCADModelDisplay
(
    QObject* parent,
    IQCADItemModel* model,
    IQCADModel3DViewer* viewer,
    QTreeView* modeltree
)
: QObject(parent),
  model_(model),
  viewer_(viewer),
  modeltree_(modeltree)
{
    viewer_->setModel(model_);

    groupingModel_ = new IQGroupingItemModel(modeltree_);
    groupingModel_->setGroupColumn(IQCADItemModel::labelCol);
    groupingModel_->setSourceModel(model_);

    modeltree_->setModel(groupingModel_);
    modeltree_->setSelectionMode(QAbstractItemView::ExtendedSelection);

    modeltree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
                modeltree_, &QTreeView::customContextMenuRequested, groupingModel_,
                [this](const QPoint& pos)
                {
                    auto idx = modeltree_->indexAt(pos);
                    groupingModel_->showContextMenu(
                        idx, modeltree_->mapToGlobal(pos), viewer_);
                }
    );

    connect( viewer_, &IQCADModel3DViewer::contextMenuRequested,
             [this](const QModelIndex& index, const QPoint &globalPos)
             {
                model_->showContextMenu(index, globalPos, viewer_);
             } );

    auto selmodel = new QItemSelectionModel(model_);
    viewer_->setSelectionModel(selmodel);
    // Note: the tree keeps its own gm-based selection model for correct index handling.
    // Bidirectional sync is handled via explicit signal/slot connections below.

    modeltree_->header()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);

    // Tree → 3D
    connect(modeltree_->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, [this](const QItemSelection& selected, const QItemSelection& deselected)
        {
            if (syncingSelection_) return;
            syncingSelection_ = true;

            auto* treeSelModel = modeltree_->selectionModel();

            std::vector<QModelIndex> deselIdxs;
            for (const auto& range : deselected)
                for (const auto& idx : range.indexes())
                {
                    if (idx.column() != 0) continue;
                    auto srcIdx = groupingModel_->mapToSource(idx);
                    if (srcIdx.isValid())
                        deselIdxs.push_back(srcIdx);
                    else
                        collectGroupChildren(groupingModel_, idx, treeSelModel,
                            QItemSelectionModel::Deselect, deselIdxs);
                }
            viewer_->externallyDeselectByModelIndices(deselIdxs);

            std::vector<QModelIndex> selIdxs;
            for (const auto& range : selected)
                for (const auto& idx : range.indexes())
                {
                    if (idx.column() != 0) continue;
                    auto srcIdx = groupingModel_->mapToSource(idx);
                    if (srcIdx.isValid())
                        selIdxs.push_back(srcIdx);
                    else
                        collectGroupChildren(groupingModel_, idx, treeSelModel,
                            QItemSelectionModel::Select, selIdxs);
                }
            viewer_->externallySelectByModelIndices(selIdxs);

            syncingSelection_ = false;
        });

    // 3D → Tree
    connect(viewer_, &IQCADModel3DViewer::viewerEntitySelected,
        this, [this](const QPersistentModelIndex& srcIdx)
        {
            if (syncingSelection_) return;
            syncingSelection_ = true;
            auto gmIdx = groupingModel_->mapFromSource(QModelIndex(srcIdx));
            if (gmIdx.isValid())
            {
                // Expand all ancestors so the item is visible in the tree
                for (auto p = gmIdx.parent(); p.isValid(); p = p.parent())
                    modeltree_->expand(p);
                modeltree_->selectionModel()->select(
                    gmIdx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                modeltree_->scrollTo(gmIdx, QAbstractItemView::EnsureVisible);
            }
            syncingSelection_ = false;
        });

    connect(viewer_, &IQCADModel3DViewer::viewerEntityDeselected,
        this, [this](const QPersistentModelIndex& srcIdx)
        {
            if (syncingSelection_) return;
            syncingSelection_ = true;
            auto gmIdx = groupingModel_->mapFromSource(QModelIndex(srcIdx));
            if (gmIdx.isValid())
                modeltree_->selectionModel()->select(
                    gmIdx, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
            syncingSelection_ = false;
        });

    connect(viewer_, &IQCADModel3DViewer::viewerSelectionCleared,
        this, [this]()
        {
            if (syncingSelection_) return;
            syncingSelection_ = true;
            modeltree_->selectionModel()->clearSelection();
            syncingSelection_ = false;
        });
}

IQVTKISCADModelDisplay::~IQVTKISCADModelDisplay()
{}

