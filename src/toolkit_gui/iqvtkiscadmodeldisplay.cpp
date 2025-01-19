#include "iqvtkiscadmodeldisplay.h"

#include <QItemSelectionModel>

#include "iqgroupingitemmodel.h"


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

    auto *gm = new IQGroupingItemModel(modeltree_);
    gm->setGroupColumn(IQCADItemModel::labelCol);
    gm->setSourceModel(model_);
    modeltree_->setModel(gm);
    // modeltree_->setModel(model_);

    modeltree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
                modeltree_, &QTreeView::customContextMenuRequested, gm,
                [this,gm](const QPoint& pos)
                {
                    auto idx = modeltree_->indexAt(pos);
                    gm->showContextMenu(
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
    modeltree_->setSelectionModel(selmodel);
}

IQVTKISCADModelDisplay::~IQVTKISCADModelDisplay()
{}

