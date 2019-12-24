#ifndef CONVERGENCEANALYSISDISPLAYER_H
#define CONVERGENCEANALYSISDISPLAYER_H

#include "base/progressdisplayer.h"
#include <vector>

namespace insight
{

class ConvergenceAnalysisDisplayer
  : public ProgressDisplayer
{
  std::string progvar_;
  std::vector<double> trackedValues_;

  int istart_, co_;
  double threshold_;

  bool converged_;

public:
  ConvergenceAnalysisDisplayer ( const std::string &progvar, double threshold = 1e-5 );

  virtual void update ( const ProgressState &pi );

  virtual bool stopRun() const;
};

typedef std::shared_ptr<ConvergenceAnalysisDisplayer>
  ConvergenceAnalysisDisplayerPtr;


}

#endif // CONVERGENCEANALYSISDISPLAYER_H
