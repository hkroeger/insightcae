#include "parametersetvisualizer.h"
#include "cadfeature.h"
#include "qoccviewwidget.h"
#include "qmodeltree.h"


namespace insight
{

CAD_ParameterSet_Visualizer::UsageTracker::UsageTracker(QModelTree* mt)
  : mt_(mt)
{
  mt->getFeatureNames(removedFeatures_);
  mt->getDatumNames(removedDatums_);
}


void CAD_ParameterSet_Visualizer::UsageTracker::cleanupModelTree()
{
  for (const std::string& sn: removedFeatures_)
  {
    mt_->onRemoveFeature( QString::fromStdString(sn) );
  }
  for (const std::string& sn: removedDatums_)
  {
    mt_->onRemoveDatum( QString::fromStdString(sn) );
  }
}


void CAD_ParameterSet_Visualizer::addDatum(const std::string& name, insight::cad::DatumPtr dat)
{
  auto i=ut_->removedDatums_.find(name);
  ut_->mt_->onAddDatum(QString::fromStdString(name), dat);
  if (i!=ut_->removedDatums_.end()) ut_->removedDatums_.erase(i);
}

void CAD_ParameterSet_Visualizer::addFeature(const std::string& name, insight::cad::FeaturePtr feat, DisplayStyle ds)
{
  auto i=ut_->removedFeatures_.find(name);
  QString qname = QString::fromStdString(name);
  ut_->mt_->onAddFeature
      (
        qname,
        feat,
        true
      );
  if (ds == Wireframe)
  {
    ut_->mt_->findFeature(qname, true)->wireframe();
  }
  if (i!=ut_->removedFeatures_.end()) ut_->removedFeatures_.erase(i);
}

void CAD_ParameterSet_Visualizer::update(const ParameterSet& ps)
{
  ParameterSet_Visualizer::update(ps);
  emit GUINeedsUpdate();
}

void CAD_ParameterSet_Visualizer::recreateVisualizationElements(UsageTracker* ut)
{
  ut_=ut;
}


}
