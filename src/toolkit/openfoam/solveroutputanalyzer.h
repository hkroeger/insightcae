#ifndef INSIGHT_SOLVEROUTPUTANALYZER_H
#define INSIGHT_SOLVEROUTPUTANALYZER_H

#include "base/outputanalyzer.h"
#include "base/progressdisplayer.h"

#include <map>
#include <memory>
#include <armadillo>

namespace insight {



class SolverOutputAnalyzer
    : public OutputAnalyzer
{
public:

#ifndef SWIG
  const static std::string
      pre_resi,
      pre_force,
      pre_conterr,
      pre_moment,
      pre_orient,
      pre_motion,
      pre_courant,
      pre_exectime,
      pre_simspeed,
      pre_deltat;
#endif

  struct CourantInfo { double mean, max; };
  struct ExecTimeInfo { double exec, wallclock; };

protected:

    double curTime_;
    std::map<std::string, double> curProgVars_;

    /**
     * name of currently tracked force output,
     * emtpy, if no force output is currently expected
     */
    std::string curforcename_;
    int curforcesection_;
    arma::mat curforcevalue_;
    std::string currbname_;

    std::shared_ptr<CourantInfo> last_courant_, last_if_courant_;
    std::shared_ptr<double> last_dt_;
    std::shared_ptr<ExecTimeInfo> last_exec_time_info_, last_last_exec_time_info_;

    std::string curLog_;

    std::shared_ptr<ActionProgress> solverActionProgress_;

public:
    SolverOutputAnalyzer ( ProgressDisplayer& pd, double endTime=1000 );

    void update (const std::string& line) override;

    bool stopRun() const override;
};



} // namespace insight

#endif // INSIGHT_SOLVEROUTPUTANALYZER_H
