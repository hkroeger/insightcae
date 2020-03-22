#include "solveroutputanalyzer.h"

#include "base/progressdisplayer.h"
#include "base/tools.h"
#include "base/units.h"
#include "base/boost_include.h"

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




SolverOutputAnalyzer::SolverOutputAnalyzer(ProgressDisplayer& pdisp)
: pdisp_(pdisp),
  curTime_(nan("NAN")),
  curforcename_(""),
  curforcesection_(1),
  currbname_("")
{
}




void SolverOutputAnalyzer::update(const std::string& line)
{
    boost::smatch match;

    boost::regex p_pattern("^ *[Pp]ressure *: *\\((.*) (.*) (.*)\\)$");
    boost::regex v_pattern("^ *[Vv]iscous *: *\\((.*) (.*) (.*)\\)$");
    boost::regex por_pattern("^ *[Pp]orous *: *\\((.*) (.*) (.*)\\)$");
    boost::regex time_pattern("^Time = (.+)$");
    boost::regex solver_pattern("^(.+): +Solving for (.+), Initial residual = (.+), Final residual = (.+), No Iterations (.+)$");
    boost::regex cont_pattern("^time step continuity errors : sum local = (.+), global = (.+), cumulative = (.+)$");
    boost::regex force_pattern("^(extendedForces|forces) (.+) (output|write):$");
    boost::regex sw_pattern("^ *[Ss]um of moments");

    boost::regex rb_pattern("Rigid-body motion of the (.+)");
    boost::regex rb_cor_pattern(" *Centre of rotation: \\((.+) (.+) (.+)\\)");
    boost::regex rb_ori_pattern(" *Orientation: \\((.+) (.+) (.+) (.+) (.+) (.+) (.+) (.+) (.+)\\)");

    boost::regex courant_pattern("^ *Courant Number mean: (.+) max: (.+)");
    boost::regex if_courant_pattern("^ *Interface Courant Number mean: (.+) max: (.+)");
    boost::regex dt_pattern(" *deltaT = (.+)");
    boost::regex exec_time_pattern(" *ExecutionTime = (.+) s  ClockTime = (.+) s");

    try
    {
        if ( boost::regex_search( line, match, sw_pattern, boost::match_default ) && !curforcename_.empty() )
        {
            curforcesection_=2;
        }
        else if ( boost::regex_search( line, match, courant_pattern, boost::match_default )  )
        {
            last_courant_.reset(new CourantInfo({ to_number<double>(match[1]), to_number<double>(match[2]) }));
        }
        else if ( boost::regex_search( line, match, if_courant_pattern, boost::match_default )  )
        {
            last_if_courant_.reset(new CourantInfo({ to_number<double>(match[1]), to_number<double>(match[2]) }));
        }
        else if ( boost::regex_search( line, match, dt_pattern, boost::match_default )  )
        {
            last_dt_.reset(new double( to_number<double>(match[1])));
        }
        else if ( boost::regex_search( line, match, exec_time_pattern, boost::match_default )  )
        {
          if (last_exec_time_info_)
          {
            last_last_exec_time_info_ = last_exec_time_info_;
          }
          last_exec_time_info_.reset(new ExecTimeInfo( { to_number<double>(match[1]), to_number<double>(match[2]) }));
        }
        else if ( boost::regex_search( line, match, rb_pattern, boost::match_default )  )
        {
            currbname_=match[1];
        }
        else if ( boost::regex_search( line, match, rb_cor_pattern, boost::match_default ) && !currbname_.empty() )
        {
          double
              cx=to_number<double>(match[1]),
              cy=to_number<double>(match[2]),
              cz=to_number<double>(match[3]);
          curProgVars_[pre_motion+currbname_+"/cx"]=cx;
          curProgVars_[pre_motion+currbname_+"/cy"]=cy;
          curProgVars_[pre_motion+currbname_+"/cz"]=cz;
        }
        else if ( boost::regex_search( line, match, rb_ori_pattern, boost::match_default ) && !currbname_.empty() )
        {
          double
              ox=std::asin(to_number<double>(match[8]))/SI::deg, // sin alpha in case of pure rot around x
              oy=std::asin(to_number<double>(match[3]))/SI::deg, // sin alpha in case of pure rot around y
              oz=std::asin(to_number<double>(match[4]))/SI::deg; // sin alpha in case of pure rot around z
          curProgVars_[pre_orient+currbname_+"/ox"]=ox;
          curProgVars_[pre_orient+currbname_+"/oy"]=oy;
          curProgVars_[pre_orient+currbname_+"/oz"]=oz;
        }
        else if ( boost::regex_search( line, match, p_pattern, boost::match_default ) && !curforcename_.empty()  )
        {
            double px=to_number<double>(match[1]);
            double py=to_number<double>(match[2]);
            double pz=to_number<double>(match[3]);
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
            double vx=to_number<double>(match[1]);
            double vy=to_number<double>(match[2]);
            double vz=to_number<double>(match[3]);
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
                pdisp_.update(
                      ProgressState(
                        curTime_,
                        curProgVars_,
                        curLog_
                        )
                      );
                curProgVars_.clear();
                curLog_.clear();
            }
            curTime_=to_number<double>(match[1]);

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
            curProgVars_[pre_resi+match[2]] = to_number<double>(match[3]);
        }
        else if ( boost::regex_search( line, match, cont_pattern, boost::match_default ) )
        {
            curProgVars_[pre_conterr+"local"] = to_number<double>(match[1]);
            curProgVars_[pre_conterr+"global"] = to_number<double>(match[2]);
            curProgVars_[pre_conterr+"cumulative"] = to_number<double>(match[3]);
        }
    }
    catch (...)
    {
      // ignore errors
    }

    curLog_ += line+"\n";

}

bool SolverOutputAnalyzer::stopRun() const
{
  return pdisp_.stopRun();
}



} // namespace insight
