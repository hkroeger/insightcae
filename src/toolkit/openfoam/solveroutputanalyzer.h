#ifndef INSIGHT_SOLVEROUTPUTANALYZER_H
#define INSIGHT_SOLVEROUTPUTANALYZER_H

#include <map>
#include <memory>
#include <armadillo>

namespace insight {

class ProgressDisplayer;

class SolverOutputAnalyzer
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
    ProgressDisplayer& pdisp_;

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

//   std::map<std::string, std::vector<arma::mat> > trackedForces_;

public:
    SolverOutputAnalyzer ( ProgressDisplayer& pdisp );

    void update ( const std::string& line );

    bool stopRun() const;
};



} // namespace insight

#endif // INSIGHT_SOLVEROUTPUTANALYZER_H
