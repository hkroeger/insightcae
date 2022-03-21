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
  CurrentExceptionContext ec("adding visualizer datum "+name);
  Q_EMIT createdDatum(QString::fromStdString(name), dat);
}

void CAD_ParameterSet_Visualizer::addFeature(const std::string& name, insight::cad::FeaturePtr feat, AIS_DisplayMode ds)
{
  CurrentExceptionContext ec("adding visualizer feature "+name);
  Q_EMIT createdFeature( QString::fromStdString(name), feat, true, ds );
}

void CAD_ParameterSet_Visualizer::update(const ParameterSet& ps)
{
  CurrentExceptionContext ex("setting parameters for parameter set visualizer");

  ParameterSet_Visualizer::update(ps);
  Q_EMIT launchVisualizationCalculation();
}

void CAD_ParameterSet_Visualizer::recreateVisualizationElements()
{
}



CAD_ParameterSet_Visualizer::CAD_ParameterSet_Visualizer()
{
  moveToThread(&asyncRebuildThread_);
  asyncRebuildThread_.start();

  connect(
        this,
        &CAD_ParameterSet_Visualizer::launchVisualizationCalculation,
        this,
        &CAD_ParameterSet_Visualizer::visualizeScheduledParameters );
}

CAD_ParameterSet_Visualizer::~CAD_ParameterSet_Visualizer()
{
  asyncRebuildThread_.quit();
  dbg()<<"waiting for visualizer thread to finish..."<<std::endl;
  asyncRebuildThread_.wait();
}


void CAD_ParameterSet_Visualizer::visualizeScheduledParameters()
{
  std::unique_lock<std::mutex> lck(vis_mtx_, std::defer_lock);

  CurrentExceptionContext ex("computing visualization of scheduled parameter set");

  if (lck.try_lock())
  {
    if (selectScheduledParameters())
    {
      try
      {
        CurrentExceptionContext ex("recreate visualization elements");

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




Multi_CAD_ParameterSet_Visualizer::Multi_CAD_ParameterSet_Visualizer()
{}




void Multi_CAD_ParameterSet_Visualizer::registerVisualizer(CAD_ParameterSet_Visualizer* vis)
{
  CurrentExceptionContext ex("registering visualizer");

  visualizers_.insert(vis);

  disconnect(vis, &CAD_ParameterSet_Visualizer::launchVisualizationCalculation, 0, 0);

  connect(
        vis,
        &CAD_ParameterSet_Visualizer::launchVisualizationCalculation,
        this,
        &CAD_ParameterSet_Visualizer::visualizeScheduledParameters );

  connect(
        vis,
        &CAD_ParameterSet_Visualizer::visualizationCalculationFinished,
        this,
        &Multi_CAD_ParameterSet_Visualizer::onSubVisualizationCalculationFinished );


//  connect(vis, &IQISCADModelContainer::beginRebuild,
//          this, &IQISCADModelContainer::beginRebuild );

  connect(vis, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelContainer::createdVariable),
          this, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelContainer::createdVariable) );
  connect(vis, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType>::of(&IQISCADModelContainer::createdVariable),
          this, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType>::of(&IQISCADModelContainer::createdVariable) );
  connect(vis, &IQISCADModelContainer::createdFeature,
          this, &IQISCADModelContainer::createdFeature);
  connect(vis, &IQISCADModelContainer::createdDatum,
          this, &IQISCADModelContainer::createdDatum);
  connect(vis, &IQISCADModelContainer::createdEvaluation,
          this, &IQISCADModelContainer::createdEvaluation );

  connect(vis, &IQISCADModelContainer::removedScalar,
          this, &IQISCADModelContainer::removedScalar);
  connect(vis, &IQISCADModelContainer::removedVector,
          this, &IQISCADModelContainer::removedVector);
  connect(vis, &IQISCADModelContainer::removedFeature,
          this, &IQISCADModelContainer::removedFeature);
  connect(vis, &IQISCADModelContainer::removedDatum,
          this, &IQISCADModelContainer::removedDatum);
  connect(vis, &IQISCADModelContainer::removedEvaluation,
          this, &IQISCADModelContainer::removedEvaluation);

//  connect(vis, &IQISCADModelContainer::finishedRebuild,
//          this, &IQISCADModelContainer::removeNonRecreatedSymbols);
}




void Multi_CAD_ParameterSet_Visualizer::unregisterVisualizer(CAD_ParameterSet_Visualizer* vis)
{
  disconnect(vis, &CAD_ParameterSet_Visualizer::launchVisualizationCalculation, 0, 0);
  disconnect(vis, &CAD_ParameterSet_Visualizer::visualizationCalculationFinished, 0, 0);

  // reconnect internally
  connect(
        vis,
        &CAD_ParameterSet_Visualizer::launchVisualizationCalculation,
        vis,
        &CAD_ParameterSet_Visualizer::visualizeScheduledParameters );

  visualizers_.erase(vis);
}

int Multi_CAD_ParameterSet_Visualizer::size() const
{
  return visualizers_.size();
}




void Multi_CAD_ParameterSet_Visualizer::recreateVisualizationElements()
{
  CurrentExceptionContext ex("re-creating all visualization elements");

  finishedVisualizers_.clear();

  for (auto& vis: visualizers_)
  {
    try
    {
      vis->currentParameters(); // might fail, because some visualizers did not yet received parameters
    }
    catch (insight::Exception& e)
    {
      continue;
    }

    vis->recreateVisualizationElements();
  }
}

bool Multi_CAD_ParameterSet_Visualizer::hasScheduledParameters() const
{
  bool any=false;
  for (auto& vis: visualizers_)
  {
    any=any || vis->hasScheduledParameters();
  }
  return any;
}


bool Multi_CAD_ParameterSet_Visualizer::selectScheduledParameters()
{
  bool any=false;
  for (auto& vis: visualizers_)
  {
    if (vis->hasScheduledParameters()) vis->clearScheduledParameters();
    any=any || vis->selectScheduledParameters();
  }
  return any;
}

void Multi_CAD_ParameterSet_Visualizer::clearScheduledParameters()
{
//  for (auto& vis: visualizers_)
//  {
//    vis->clearScheduledParameters();
//  }
}


void Multi_CAD_ParameterSet_Visualizer::onSubVisualizationCalculationFinished()
{
  finishedVisualizers_.insert(
        qobject_cast<CAD_ParameterSet_Visualizer*>(sender()) );
  if (finishedVisualizers_.size()==visualizers_.size())
  {
    Q_EMIT visualizationCalculationFinished();
  }
}

}
