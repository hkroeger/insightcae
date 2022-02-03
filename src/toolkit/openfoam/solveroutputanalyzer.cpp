#include "solveroutputanalyzer.h"

#include <memory>

#include "base/progressdisplayer.h"
#include "base/tools.h"
#include "base/units.h"
#include "base/boost_include.h"


using namespace std;
using namespace boost;

namespace insight {




const std::string SolverOutputAnalyzer::pre_resi="residual/";
const std::string SolverOutputAnalyzer::pre_force="force/";
const std::string SolverOutputAnalyzer::pre_moment="moment/";
const std::string SolverOutputAnalyzer::pre_conterr="continuity_error/";
const std::string SolverOutputAnalyzer::pre_orient="rb_orientation/";
const std::string SolverOutputAnalyzer::pre_motion="rb_motion/";
const std::string SolverOutputAnalyzer::pre_courant="courant_no/";
const std::string SolverOutputAnalyzer::pre_exectime="exec_time/";
const std::string SolverOutputAnalyzer::pre_simspeed="sim_speed/";
const std::string SolverOutputAnalyzer::pre_deltat="delta_t/";




SolverOutputAnalyzer::SolverOutputAnalyzer(ProgressDisplayer& pd, double endTime)
: OutputAnalyzer(&pd),
  curTime_(nan("NAN")),
  curforcename_(""),
  curforcesection_(1),
  currbname_(""),
  p_pattern("^ *[Pp]ressure *: *\\((.*) (.*) (.*)\\)$"),
  v_pattern("^ *[Vv]iscous *: *\\((.*) (.*) (.*)\\)$"),
  por_pattern("^ *[Pp]orous *: *\\((.*) (.*) (.*)\\)$"),
  time_pattern("^Time = (.+)$"),
  solver_pattern("^(.+): +Solving for (.+), Initial residual = (.+), Final residual = (.+), No Iterations (.+)$"),
  cont_pattern("^time step continuity errors : sum local = (.+), global = (.+), cumulative = (.+)$"),
  force_pattern("^(extendedForces|forces) (.+) (output|write):$"),
  sw_pattern("^ *[Ss]um of moments"),
  rb_pattern("Rigid-body motion of the (.+)"),
  rb_cor_pattern(" *Centre of rotation: \\((.+) (.+) (.+)\\)"),
  rb_ori_pattern(" *Orientation: \\((.+) (.+) (.+) (.+) (.+) (.+) (.+) (.+) (.+)\\)"),
  courant_pattern("^ *Courant Number mean: (.+) max: (.+)"),
  if_courant_pattern("^ *Interface Courant Number mean: (.+) max: (.+)"),
  dt_pattern(" *deltaT = (.+)"),
  exec_time_pattern(" *ExecutionTime = (.+) s  ClockTime = (.+) s")
{
  solverActionProgress_ = std::make_shared<ActionProgress>
      (
        pd.forkNewAction(endTime, "Solver run")
      );
}




void SolverOutputAnalyzer::update(const std::string& line)
{
    boost::smatch match;

    try
    {
        if ( boost::regex_search( line, match, sw_pattern, boost::match_default ) && !curforcename_.empty() )
        {
            curforcesection_=2;
        }
        else if ( boost::regex_search( line, match, courant_pattern, boost::match_default )  )
        {
            last_courant_.reset(new CourantInfo({ toNumber<double>(match[1]), toNumber<double>(match[2]) }));
        }
        else if ( boost::regex_search( line, match, if_courant_pattern, boost::match_default )  )
        {
            last_if_courant_.reset(new CourantInfo({ toNumber<double>(match[1]), toNumber<double>(match[2]) }));
        }
        else if ( boost::regex_search( line, match, dt_pattern, boost::match_default )  )
        {
            last_dt_.reset(new double( toNumber<double>(match[1])));
        }
        else if ( boost::regex_search( line, match, exec_time_pattern, boost::match_default )  )
        {
          if (last_exec_time_info_)
          {
            last_last_exec_time_info_ = last_exec_time_info_;
          }
          last_exec_time_info_.reset(new ExecTimeInfo( { toNumber<double>(match[1]), toNumber<double>(match[2]) }));
        }
        else if ( boost::regex_search( line, match, rb_pattern, boost::match_default )  )
        {
            currbname_=match[1];
        }
        else if ( boost::regex_search( line, match, rb_cor_pattern, boost::match_default ) && !currbname_.empty() )
        {
          double
              cx=toNumber<double>(match[1]),
              cy=toNumber<double>(match[2]),
              cz=toNumber<double>(match[3]);
          curProgVars_[pre_motion+currbname_+"/cx"]=cx;
          curProgVars_[pre_motion+currbname_+"/cy"]=cy;
          curProgVars_[pre_motion+currbname_+"/cz"]=cz;
        }
        else if ( boost::regex_search( line, match, rb_ori_pattern, boost::match_default ) && !currbname_.empty() )
        {
          double
              ox=std::asin(toNumber<double>(match[8]))/SI::deg, // sin alpha in case of pure rot around x
              oy=std::asin(toNumber<double>(match[3]))/SI::deg, // sin alpha in case of pure rot around y
              oz=std::asin(toNumber<double>(match[4]))/SI::deg; // sin alpha in case of pure rot around z
          curProgVars_[pre_orient+currbname_+"/ox"]=ox;
          curProgVars_[pre_orient+currbname_+"/oy"]=oy;
          curProgVars_[pre_orient+currbname_+"/oz"]=oz;
        }
        else if ( boost::regex_search( line, match, p_pattern, boost::match_default ) && !curforcename_.empty()  )
        {
            double px=toNumber<double>(match[1]);
            double py=toNumber<double>(match[2]);
            double pz=toNumber<double>(match[3]);
            if (curforcesection_==1)
            {
                // force
                curforcevalue_(0)=px;
                curforcevalue_(1)=py;
                curforcevalue_(2)=pz;
            }
            else if (curforcesection_==2)
            {
                // moment
                curforcevalue_(6)=px;
                curforcevalue_(7)=py;
                curforcevalue_(8)=pz;
            }
        }
        else if ( boost::regex_search( line, match, v_pattern, boost::match_default ) && !curforcename_.empty()  )
        {
            double vx=toNumber<double>(match[1]);
            double vy=toNumber<double>(match[2]);
            double vz=toNumber<double>(match[3]);
            if (curforcesection_==1)
            {
                // force
                curforcevalue_(3)=vx;
                curforcevalue_(4)=vy;
                curforcevalue_(5)=vz;
            }
            else if (curforcesection_==2)
            {
                // moment
                curforcevalue_(9)=vx;
                curforcevalue_(10)=vy;
                curforcevalue_(11)=vz;
            }
        }
        else if ( boost::regex_search( line, match, por_pattern, boost::match_default ) && !curforcename_.empty()  )
        {
        }
        else if ( boost::regex_search( line, match, force_pattern, boost::match_default ) )
        {
            if (!curforcename_.empty()) //(curforcesection_==2)
            {
                // end active

                // store
                curProgVars_[pre_force+curforcename_+"/fpx"]=curforcevalue_(0);
                curProgVars_[pre_force+curforcename_+"/fpy"]=curforcevalue_(1);
                curProgVars_[pre_force+curforcename_+"/fpz"]=curforcevalue_(2);
                curProgVars_[pre_force+curforcename_+"/fvx"]=curforcevalue_(3);
                curProgVars_[pre_force+curforcename_+"/fvy"]=curforcevalue_(4);
                curProgVars_[pre_force+curforcename_+"/fvz"]=curforcevalue_(5);
                curProgVars_[pre_moment+curforcename_+"/mpx"]=curforcevalue_(6);
                curProgVars_[pre_moment+curforcename_+"/mpy"]=curforcevalue_(7);
                curProgVars_[pre_moment+curforcename_+"/mpz"]=curforcevalue_(8);
                curProgVars_[pre_moment+curforcename_+"/mvx"]=curforcevalue_(9);
                curProgVars_[pre_moment+curforcename_+"/mvy"]=curforcevalue_(10);
                curProgVars_[pre_moment+curforcename_+"/mvz"]=curforcevalue_(11);

                // reset tracker
                curforcename_="";
            }

            curforcename_=match[2];
            curforcesection_=1;
            curforcevalue_=arma::zeros(12);
        }
        else if ( boost::regex_search( line, match, time_pattern, boost::match_default ) )  // new time step begins
        {
            if (!curforcename_.empty()) //(curforcesection_==2)
            {
                // end active

                // store
                curProgVars_[pre_force+curforcename_+"/fpx"]=curforcevalue_(0);
                curProgVars_[pre_force+curforcename_+"/fpy"]=curforcevalue_(1);
                curProgVars_[pre_force+curforcename_+"/fpz"]=curforcevalue_(2);
                curProgVars_[pre_force+curforcename_+"/fvx"]=curforcevalue_(3);
                curProgVars_[pre_force+curforcename_+"/fvy"]=curforcevalue_(4);
                curProgVars_[pre_force+curforcename_+"/fvz"]=curforcevalue_(5);
                curProgVars_[pre_moment+curforcename_+"/mpx"]=curforcevalue_(6);
                curProgVars_[pre_moment+curforcename_+"/mpy"]=curforcevalue_(7);
                curProgVars_[pre_moment+curforcename_+"/mpz"]=curforcevalue_(8);
                curProgVars_[pre_moment+curforcename_+"/mvx"]=curforcevalue_(9);
                curProgVars_[pre_moment+curforcename_+"/mvy"]=curforcevalue_(10);
                curProgVars_[pre_moment+curforcename_+"/mvz"]=curforcevalue_(11);

                // reset tracker
                curforcename_="";
                curforcesection_=1;
                curforcevalue_=arma::zeros(12);
            }

            if (curTime_ == curTime_)
            {
                progress_->update(
                      ProgressState(
                        curTime_,
                        curProgVars_,
                        curLog_
                        )
                      );
                curProgVars_.clear();
                curLog_.clear();

                if (solverActionProgress_) solverActionProgress_->stepTo(curTime_);
            }
            curTime_=toNumber<double>(match[1]);

            if (last_courant_)
            {
              curProgVars_[pre_courant+"mean"]=last_courant_->mean;
              curProgVars_[pre_courant+"max"]=last_courant_->max;
            }
            if (last_if_courant_)
            {
              curProgVars_[pre_courant+"interface_mean"]=last_if_courant_->mean;
              curProgVars_[pre_courant+"interface_max"]=last_if_courant_->max;
            }
            if (last_dt_)
            {
              curProgVars_[pre_deltat+"delta_t"]=*last_dt_;
            }
            if (last_exec_time_info_ && last_last_exec_time_info_)
            {
              curProgVars_[pre_exectime+"delta_exec_time"]=last_exec_time_info_->exec - last_last_exec_time_info_->exec;
              curProgVars_[pre_exectime+"delta_clock_time"]=last_exec_time_info_->wallclock - last_last_exec_time_info_->wallclock;
            }
            if (last_dt_ && last_exec_time_info_ && last_last_exec_time_info_)
            {
              curProgVars_[pre_simspeed+"sim_second_per_wall_clock_hour"]=3600.* (*last_dt_) / (last_exec_time_info_->wallclock - last_last_exec_time_info_->wallclock);
              curProgVars_[pre_simspeed+"sim_second_per_exec_hour"]=3600.* (*last_dt_) / (last_exec_time_info_->exec - last_last_exec_time_info_->exec);
            }

        }
        else if ( boost::regex_search( line, match, solver_pattern, boost::match_default ) )
        {
            curProgVars_[pre_resi+match[2]] = toNumber<double>(match[3]);
        }
        else if ( boost::regex_search( line, match, cont_pattern, boost::match_default ) )
        {
            curProgVars_[pre_conterr+"local"] = toNumber<double>(match[1]);
            curProgVars_[pre_conterr+"global"] = toNumber<double>(match[2]);
            curProgVars_[pre_conterr+"cumulative"] = toNumber<double>(match[3]);
        }
    }
    catch (...)
    {
      // ignore errors
    }

//    curLog_ += line+"\n";

}

bool SolverOutputAnalyzer::stopRun() const
{
  return progress_->stopRun();
}




} // namespace insight
