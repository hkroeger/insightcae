#include "iqvtkiscadmodeldisplay.h"


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
    modeltree_->setModel(model_);

    modeltree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
                modeltree_, &QTreeView::customContextMenuRequested, model_,
                [this](const QPoint& pos)
                {
                    auto idx = modeltree_->indexAt(pos);
                    model_->showContextMenu(idx, modeltree_->mapToGlobal(pos));
                }
    );

    connect( viewer_, &IQCADModel3DViewer::contextMenuRequested,
             model_, &IQCADItemModel::showContextMenu );
}

IQVTKISCADModelDisplay::~IQVTKISCADModelDisplay()
{}
