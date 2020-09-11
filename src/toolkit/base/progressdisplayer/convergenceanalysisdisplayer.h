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

  void update ( const ProgressState &pi ) override;
  void setActionProgressValue(const std::string& path, double value) override;
  void setMessageText(const std::string& path, const std::string& message) override;
  void finishActionProgress(const std::string& path) override;
  void reset() override;

  bool stopRun() const override;
};

typedef std::shared_ptr<ConvergenceAnalysisDisplayer>
  ConvergenceAnalysisDisplayerPtr;


}

#endif // CONVERGENCEANALYSISDISPLAYER_H
