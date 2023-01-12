#include "openradiossoutputanalyzer.h"

#include "base/exception.h"

namespace insight {

OpenRadiossOutputAnalyzer::OpenRadiossOutputAnalyzer
(
        double endTime,
        ProgressDisplayer* parentProgress,
        AnimWriteCallback animWriteCallback
)
    : progressLinePattern_("NC= *([^ ]+) T= *([^ ]+) DT= *([^ ]+) ERR= *([^ ]+)% DM/M= *([^ ]+)"),
      timePattern_("ELAPSED TIME= *([^ ]+) s  REMAINING TIME= *([^ ]+) s"),
      animWritePattern_("ANIMATION FILE: ([^ ]+) WRITTEN"),
      animName_("^(.+A)([0-9]+)$"),
      endTime_(endTime),
      progr_( parentProgress?
            std::make_shared<ActionProgress>(parentProgress->forkNewAction(endTime, "OpenRadioss solver"))
            : nullptr
           ),
      animWriteCallback_(animWriteCallback)
{}

void OpenRadiossOutputAnalyzer::update(const std::string &line)
{
    boost::smatch match;

    if ( boost::regex_search(line, match, progressLinePattern_, boost::match_default) )
    {
        double t=boost::lexical_cast<double>(match[2]);
        double dt=boost::lexical_cast<double>(match[3]);
        double err=boost::lexical_cast<double>(match[4]);
        double dmBym=boost::lexical_cast<double>(match[5]);
        if (progr_)
        {
            progr_->stepTo(t);

            if (curPS_)
            {
                progr_->update(*curPS_);
            }
            else
            {
                curPS_.reset(new ProgressState);
            }


            curPS_->first = t;
            curPS_->second["solver/deltaT"]=dt;
            curPS_->second["error/err"]=err;
            curPS_->second["massError/dmBym"]=dmBym;
        }
    }
    else if ( boost::regex_search(line, match, timePattern_, boost::match_default) )
    {
        double tElaps=boost::lexical_cast<double>(match[1]);
        double tRemain=boost::lexical_cast<double>(match[2]);
        if (progr_ && curPS_)
        {
            curPS_->second["runtime/elapsedTime"]=tElaps;
            curPS_->second["runtime/remainingTime"]=tRemain;
        }
    }
    else if ( boost::regex_search(line, match, animWritePattern_, boost::match_default) )
    {
        if (animWriteCallback_)
        {
            std::string fname(match[1]);

            boost::smatch m2;
            insight::assertion(
                boost::regex_search(fname, m2, animName_, boost::match_default),
                "unexpected animation file name");
            animWriteCallback_(fname, boost::lexical_cast<int>(m2[2]));
        }
    }
}

} // namespace insight
