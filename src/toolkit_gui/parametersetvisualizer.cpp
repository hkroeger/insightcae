#include <QDebug>

#include "parametersetvisualizer.h"
#include "cadfeature.h"
#include "qoccviewwidget.h"
#include "qmodeltree.h"
#include "qmodelstepitem.h"


namespace insight
{



void CAD_ParameterSet_Visualizer::addDatum(const std::string& name, insight::cad::DatumPtr dat)
{
  Q_EMIT createdDatum(QString::fromStdString(name), dat);
}

void CAD_ParameterSet_Visualizer::addFeature(const std::string& name, insight::cad::FeaturePtr feat, AIS_DisplayMode ds)
{
  Q_EMIT createdFeature( QString::fromStdString(name), feat, true, ds );
}

void CAD_ParameterSet_Visualizer::update(const ParameterSet& ps)
{
  ParameterSet_Visualizer::update(ps);
}

void CAD_ParameterSet_Visualizer::recreateVisualizationElements()
{
}


std::mutex vis_mtx;

CAD_ParameterSet_Visualizer::CAD_ParameterSet_Visualizer()
{
  moveToThread(&asyncRebuildThread_);
  asyncRebuildThread_.start();
}

CAD_ParameterSet_Visualizer::~CAD_ParameterSet_Visualizer()
{
  asyncRebuildThread_.quit();
  qDebug()<<"waiting for visualizer thread to finish...";
  asyncRebuildThread_.wait();
}


void CAD_ParameterSet_Visualizer::visualizeScheduledParameters()
{
  std::unique_lock<std::mutex> lck(vis_mtx, std::defer_lock);

  if (lck.try_lock())
  {
    if (selectScheduledParameters())
    {
      try
      {
        Q_EMIT beginRebuild();

        recreateVisualizationElements();

        Q_EMIT finishedRebuild();

        insight::cad::cache.printSummary(std::cout);
      }
      catch (insight::Exception& e)
      {
        cerr<<"Warning: could not rebuild visualization."
              " Error was:"<<e
           <<endl;
      }

      clearScheduledParameters();
    }
    Q_EMIT visualizationCalculationFinished();
  }

}



}
