#include "iqvtkparametersetdisplay.h"
#include "cadparametersetvisualizer.h"

IQVTKParameterSetDisplay::IQVTKParameterSetDisplay
(
    QObject* parent,
    IQCADModel3DViewer* viewer,
    QTreeView* modeltree
)
: IQVTKISCADModelDisplay(
      parent,
      new IQCADItemModel,
      viewer, modeltree
      )
{
    model()->setParent(this);
}



IQVTKParameterSetDisplay::~IQVTKParameterSetDisplay()
{}


//void IQVTKParameterSetDisplay::connectVisualizer(std::shared_ptr<insight::CADParameterSetVisualizer> viz)
//{
//  if (viz)
//  {
//    modeltree_->connectGenerator(viz.get());
//  }
//}

//void IQVTKParameterSetDisplay::disconnectVisualizer(std::shared_ptr<insight::CADParameterSetVisualizer> viz)
//{
//  if (viz)
//  {
//    modeltree_->disconnectModel(viz.get());
//  }
//}
