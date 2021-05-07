#include "visualizerthread.h"

#include <mutex>

#include "parametereditorwidget.h"

std::mutex vis_mtx;

void VisualizerThread::run()
{
//  try
//  {
//    std::lock_guard<std::mutex> lck(vis_mtx);

//    insight::CAD_ParameterSet_Visualizer::UsageTracker ut(psd_->modeltree_);

//    for (auto& vz: psd_->visualizers_)
//    {
//      vz->recreateVisualizationElements(&ut);
//    }

//    ut.cleanupModelTree();

//    insight::cad::cache.printSummary(std::cout);
//  }
//  catch (insight::Exception& e)
//  {
//    cerr<<"Warning: could not rebuild visualization."
//          " Error was:"<<e
//       <<endl;
//  }
}

VisualizerThread::VisualizerThread(ParameterSetDisplay* psd)
  : QThread(psd), psd_(psd)
{
}
