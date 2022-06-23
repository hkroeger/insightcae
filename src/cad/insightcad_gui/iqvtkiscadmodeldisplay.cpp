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
}

IQVTKISCADModelDisplay::~IQVTKISCADModelDisplay()
{}
