#include "casolveroutputanalyzer.h"


using namespace std;
using namespace boost;

namespace insight {



std::ostream &operator<<(std::ostream& os, const Message& msg)
{
  os<<std::string(80, '=')<<"\n";
  switch(msg.msgType_)
  {
    case Message::Warning: os << "Warning: "; break;
    case Message::Exception: os << "EXCEPTION: "; break;
    case Message::None: os << "(uncategorized message): "; break;
  }
  os<<msg.msgCode_<<"\n";
  os<<static_cast<const std::string&>(msg);
  os<<std::string(80, '-')<<"\n";
  return os;
}



CASolverOutputAnalyzer::CASolverOutputAnalyzer ( ProgressDisplayer& pd, double endTime )
: OutputAnalyzer(&pd),
  re_excep("! <EXCEPTION> <(.*)>"),
  re_alarm("! <A> <(.*)>"),
  re_msgstr("! (.*) !"),
  re_msgdelim("!-----------"),
  re_instant("Instant de calcul:  (.*)$"),
  endTime_(endTime)
{
  solverActionProgress_ = std::make_shared<ActionProgress>
      (
        pd.forkNewAction(endTime, "Solver run")
      );
}



void CASolverOutputAnalyzer::update (const std::string& line)
{
  smatch groups;

  if (currentMessage_)
  {
    if (currentMessage_->msgType_==Message::None)
    {
      // next line should determine type
      if (regex_search(line, groups, re_excep, boost::match_default))
      {
        currentMessage_->msgType_ = Message::Exception;
        currentMessage_->msgCode_ = groups[1];
      }
      else if (regex_search(line, groups, re_alarm, boost::match_default))
      {
        currentMessage_->msgType_ = Message::Warning;
        currentMessage_->msgCode_ = groups[1];
      }
      else
      {
        // no type info: obviously the log started with the end mark of a message
        currentMessage_.reset();
      }
    }
    else if (regex_search(line, groups, re_msgstr, boost::match_default))
    {
      *currentMessage_ += trim_copy(std::string(groups[1]))+"\n";
    }
  }

  if (regex_search(line, re_msgdelim, boost::match_default))
  {
    if (currentMessage_) // end of msg
    {
      std::cout<<*currentMessage_;
      messages_.push_back(std::move(currentMessage_));
    }
    else // begin of msg
    {
      currentMessage_.reset(new Message);
    }
  }

  else if (regex_search(line, groups, re_instant, boost::match_default))
  {
    double curTime = lexical_cast<double>(groups[1]);
//    progress_->update(
//          ProgressState(
//            curTime,
//            {},
//            ""
//            )
//          );
    if (solverActionProgress_) solverActionProgress_->stepTo(curTime);
  }
}



bool CASolverOutputAnalyzer::stopRun() const
{
  return false;
}



} // namespace insight
