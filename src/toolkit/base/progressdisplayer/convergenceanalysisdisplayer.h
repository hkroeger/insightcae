#ifndef CONVERGENCEANALYSISDISPLAYER_H
#define CONVERGENCEANALYSISDISPLAYER_H

#include "base/factory.h"
#include "base/progressdisplayer.h"

#include <memory>
#include <string>
#include <vector>

namespace insight
{

class ConvergenceByVariance;


/**
 * Parameters of the convergence criterion of ConvergenceAnalysisDisplayer.
 * Can be set up once and reused for several displayers, e.g.
 *
 *   auto cp = ConvergenceAnalysisParameters()
 *               .set_threshold(1e-3)
 *               .set_minWindow(0.5, ConvergenceAnalysisParameters::TimeSpan);
 *   for (const auto& pv: {"fx", "fy"})
 *     installConvergenceAnalysis(
 *       std::make_shared<ConvergenceAnalysisDisplayer>(pv, cp) );
 */
struct ConvergenceAnalysisParameters
{
  enum WindowSizeUnit { Samples, TimeSpan };

  /// CoV threshold
  double threshold = 1e-2;

  /// trailing window size as fraction of the number of acquired samples
  double fraction = 0.2;

  /// minimum size of the trailing window, see minWindowUnit
  double minWindow = 50;

  /// Samples: minWindow is a number of samples,
  /// TimeSpan: minWindow is the time span covered by the window
  WindowSizeUnit minWindowUnit = Samples;

  ConvergenceAnalysisParameters& set_threshold(double v)
  { threshold=v; return *this; }

  ConvergenceAnalysisParameters& set_fraction(double v)
  { fraction=v; return *this; }

  ConvergenceAnalysisParameters& set_minWindow(double v, WindowSizeUnit unit = Samples)
  { minWindow=v; minWindowUnit=unit; return *this; }
};


/**
 * Monitors one progress variable and demands a stop of the run,
 * as soon as it is converged.
 *
 * The convergence measure is the coefficient of variation (CoV) of
 * the progress variable in a trailing window, see ConvergenceByVariance.
 * The variable is considered converged (see ConvergenceAnalysisParameters), if
 *  - the CoV is below threshold and
 *  - the trailing window is large enough (avoids false detections
 *    during the start-up phase): depending on the selected unit, it
 *    contains at least minWindow samples (Samples) or covers at least
 *    a time span of minWindow (TimeSpan, time as given in the progress
 *    state, i.e. time or iteration; additionally >=2 samples required).
 * stopIsDemanded() reflects the state after the latest sample
 * (it is reset, if the variable diverges again).
 */
class ConvergenceAnalysisDisplayer
  : public ProgressDisplayer
{
public:
  typedef ConvergenceAnalysisParameters Parameters;

private:
  std::string progvar_;
  Parameters p_;

  std::unique_ptr<ConvergenceByVariance> cbv_;
  std::vector<double> times_; // time of each acquired sample

  bool converged_;

public:
  /**
   * @param progvar name of the progress variable to monitor
   * @param p       convergence criterion
   */
  ConvergenceAnalysisDisplayer (
      const std::string &progvar,
      const Parameters& p = Parameters() );

  CREATE_FUNCTION(ConvergenceAnalysisDisplayer);

  const Parameters& parameters() const;
  ~ConvergenceAnalysisDisplayer();

  void update ( const ProgressState &pi ) override;
  void logMessage(const std::string& line) override;
  void setActionProgressValue(const std::string& path, double value) override;
  void setMessageText(const std::string& path, const std::string& message) override;
  void finishActionProgress(const std::string& path) override;
  void reset() override;

  bool stopIsDemanded() const override;
};

typedef std::shared_ptr<ConvergenceAnalysisDisplayer>
  ConvergenceAnalysisDisplayerPtr;


}

#endif // CONVERGENCEANALYSISDISPLAYER_H
