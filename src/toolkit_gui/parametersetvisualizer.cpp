#include "parametersetvisualizer.h"
#include "cadfeature.h"
#include "qoccviewwidget.h"
#include "qmodeltree.h"


namespace insight
{


void CAD_ParameterSet_Visualizer::updateVisualizationElements(QoccViewWidget* vw, QModelTree* mt)
{
  mt_=mt;

  insight::cad::cache.initRebuild();

  mt->getFeatureNames(removedFeatures_);
  mt->getDatumNames(removedDatums_);

  recreateVisualizationElements(vw);

  for (const std::string& sn: removedFeatures_)
  {
    mt->onRemoveFeature( QString::fromStdString(sn) );
  }
  for (const std::string& sn: removedDatums_)
  {
    mt->onRemoveDatum( QString::fromStdString(sn) );
  }

  insight::cad::cache.finishRebuild();
}

void CAD_ParameterSet_Visualizer::addDatum(const std::string& name, insight::cad::DatumPtr dat)
{
  auto i=removedDatums_.find(name);
  mt_->onAddDatum(QString::fromStdString(name), dat);
  if (i!=removedDatums_.end()) removedDatums_.erase(i);
}

void CAD_ParameterSet_Visualizer::addFeature(const std::string& name, insight::cad::FeaturePtr feat)
{
  auto i=removedFeatures_.find(name);
  mt_->onAddFeature(QString::fromStdString(name), feat, true);
  if (i!=removedFeatures_.end()) removedFeatures_.erase(i);
}

void CAD_ParameterSet_Visualizer::recreateVisualizationElements(QoccViewWidget* )
{
}


}
