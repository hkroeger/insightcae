#include <QDebug>

#include "cadparametersetvisualizer.h"
#include "cadfeature.h"
#include "qmodeltree.h"
#include "qmodelstepitem.h"

#include "iqiscadmodelrebuilder.h"


namespace insight
{



void CADParameterSetVisualizer::addDatum(const std::string& name, insight::cad::DatumPtr dat)
{
  CurrentExceptionContext ec("adding visualizer datum "+name);
  Q_EMIT createdDatum(QString::fromStdString(name), dat);
}

void CADParameterSetVisualizer::addFeature(const std::string& name, insight::cad::FeaturePtr feat, AIS_DisplayMode ds)
{
  CurrentExceptionContext ec("adding visualizer feature "+name);
  Q_EMIT createdFeature( QString::fromStdString(name), feat, true, ds );
}

void CADParameterSetVisualizer::update(const ParameterSet& ps)
{
  CurrentExceptionContext ex("setting parameters for parameter set visualizer");

  ParameterSetVisualizer::update(ps);
  Q_EMIT launchVisualizationCalculation(); // executed in dedicated thread
}

void CADParameterSetVisualizer::recreateVisualizationElements()
{
}



CADParameterSetVisualizer::CADParameterSetVisualizer()
{
  moveToThread(&asyncRebuildThread_);
  asyncRebuildThread_.start();

  connect(
        this,
        &CADParameterSetVisualizer::launchVisualizationCalculation,
        this,
        &CADParameterSetVisualizer::visualizeScheduledParameters );
}

CADParameterSetVisualizer::~CADParameterSetVisualizer()
{
  asyncRebuildThread_.quit();
  dbg()<<"waiting for visualizer thread to finish..."<<std::endl;
  asyncRebuildThread_.wait();
}

void CADParameterSetVisualizer::setModel(IQCADItemModel *model)
{
    model_=model;
}

IQCADItemModel *CADParameterSetVisualizer::model()
{
    return model_;
}


void CADParameterSetVisualizer::visualizeScheduledParameters()
{
  std::unique_lock<std::mutex> lck(vis_mtx_, std::defer_lock);

  CurrentExceptionContext ex("computing visualization of scheduled parameter set");

  insight::assertion(
              model_!=nullptr,
              "internal error: the output CAD model is unset!" );

  if (lck.try_lock())
  {
    if (selectScheduledParameters())
    {
      try
      {
        CurrentExceptionContext ex("recreate visualization elements");

        IQISCADModelRebuilder rb(model_, {this});
        recreateVisualizationElements();
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







MultiCADParameterSetVisualizer::MultiCADParameterSetVisualizer()
{}




void MultiCADParameterSetVisualizer::registerVisualizer(CADParameterSetVisualizer* vis)
{
  CurrentExceptionContext ex("registering visualizer");

  visualizers_.insert(vis);

  disconnect(vis, &CADParameterSetVisualizer::launchVisualizationCalculation, 0, 0);

  connect(
        vis,
        &CADParameterSetVisualizer::launchVisualizationCalculation,
        this,
        &CADParameterSetVisualizer::visualizeScheduledParameters );

  connect(
        vis,
        &CADParameterSetVisualizer::visualizationCalculationFinished,
        this,
        &MultiCADParameterSetVisualizer::onSubVisualizationCalculationFinished );


  connect(vis, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelGenerator::createdVariable),
          this, QOverload<const QString&,insight::cad::ScalarPtr>::of(&IQISCADModelGenerator::createdVariable) );
  connect(vis, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType>::of(&IQISCADModelGenerator::createdVariable),
          this, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType>::of(&IQISCADModelGenerator::createdVariable) );
  connect(vis, &IQISCADModelGenerator::createdFeature,
          this, &IQISCADModelGenerator::createdFeature);
  connect(vis, &IQISCADModelGenerator::createdDatum,
          this, &IQISCADModelGenerator::createdDatum);
  connect(vis, &IQISCADModelGenerator::createdEvaluation,
          this, &IQISCADModelGenerator::createdEvaluation );

}




void MultiCADParameterSetVisualizer::unregisterVisualizer(CADParameterSetVisualizer* vis)
{
  disconnect(vis, &CADParameterSetVisualizer::launchVisualizationCalculation, 0, 0);
  disconnect(vis, &CADParameterSetVisualizer::visualizationCalculationFinished, 0, 0);

  // reconnect internally
  connect(
        vis,
        &CADParameterSetVisualizer::launchVisualizationCalculation,
        vis,
        &CADParameterSetVisualizer::visualizeScheduledParameters );

  visualizers_.erase(vis);
}

int MultiCADParameterSetVisualizer::size() const
{
  return visualizers_.size();
}




void MultiCADParameterSetVisualizer::recreateVisualizationElements()
{
  CurrentExceptionContext ex("re-creating all visualization elements");

  insight::assertion(
              model()!=nullptr,
              "internal error: the output CAD model is unset!" );

  finishedVisualizers_.clear();

  QList<IQISCADModelGenerator*> gens;
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

    gens.append(vis);
  }

  IQISCADModelRebuilder rb(model(), gens);
  for (auto& vis: visualizers_)
  {
    vis->recreateVisualizationElements();
  }
}

bool MultiCADParameterSetVisualizer::hasScheduledParameters() const
{
  bool any=false;
  for (auto& vis: visualizers_)
  {
    any=any || vis->hasScheduledParameters();
  }
  return any;
}


bool MultiCADParameterSetVisualizer::selectScheduledParameters()
{
  bool any=false;
  for (auto& vis: visualizers_)
  {
    if (vis->hasScheduledParameters()) vis->clearScheduledParameters();
    any=any || vis->selectScheduledParameters();
  }
  return any;
}

void MultiCADParameterSetVisualizer::clearScheduledParameters()
{
//  for (auto& vis: visualizers_)
//  {
//    vis->clearScheduledParameters();
//  }
}


void MultiCADParameterSetVisualizer::onSubVisualizationCalculationFinished()
{
  finishedVisualizers_.insert(
        qobject_cast<CADParameterSetVisualizer*>(sender()) );
  if (finishedVisualizers_.size()==visualizers_.size())
  {
    Q_EMIT visualizationCalculationFinished();
  }
}

}
