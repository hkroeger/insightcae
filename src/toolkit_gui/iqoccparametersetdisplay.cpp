#include "iqoccparametersetdisplay.h"

#include "qoccviewwidget.h"
#include "qmodeltree.h"
#include "cadparametersetvisualizer.h"

IQOCCParameterSetDisplay::IQOCCParameterSetDisplay
(
    QObject* parent,
    QoccViewWidget* viewer,
    QModelTree* modeltree
)
  : QObject(parent),
    viewer_(viewer),
    modeltree_(modeltree)
{}

void IQOCCParameterSetDisplay::connectVisualizer(std::shared_ptr<insight::CADParameterSetVisualizer> viz)
{
  if (viz)
  {
    modeltree_->connectGenerator(viz.get());
  }
}

void IQOCCParameterSetDisplay::disconnectVisualizer(std::shared_ptr<insight::CADParameterSetVisualizer> viz)
{
  if (viz)
  {
    modeltree_->disconnectGenerator(viz.get());
  }
}
