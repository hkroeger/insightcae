#ifndef INSIGHT_SOLVEROUTPUTANALYZER_H
#define INSIGHT_SOLVEROUTPUTANALYZER_H

#include "base/outputanalyzer.h"
#include "base/progressdisplayer.h"

#include <map>
#include <memory>
#include <armadillo>

#include "boost/regex.hpp"

namespace insight {



class SolverOutputAnalyzer
    : public OutputAnalyzer
{
public:

#ifndef SWIG
  const static std::string
      pre_iter,
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

    boost::regex p_pattern;
    boost::regex v_pattern;
    boost::regex por_pattern;
    boost::regex time_pattern;
    boost::regex solver_pattern;
    boost::regex cont_pattern;
    boost::regex force_pattern;
    boost::regex sw_pattern;

    boost::regex rb_pattern;
    boost::regex rb_cor_pattern;
    boost::regex rb_ori_pattern;

    boost::regex courant_pattern;
    boost::regex if_courant_pattern;
    boost::regex dt_pattern;
    boost::regex exec_time_pattern;

    boost::regex pimple_iter_pattern;


public:
    SolverOutputAnalyzer ( ProgressDisplayer& pd, double endTime=1000 );

    void update (const std::string& line) override;

    bool stopRun() const override;
};



} // namespace insight

#endif // INSIGHT_SOLVEROUTPUTANALYZER_H
