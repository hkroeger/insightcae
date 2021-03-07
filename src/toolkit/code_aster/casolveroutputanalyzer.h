#ifndef INSIGHT_CASOLVEROUTPUTANALYZER_H
#define INSIGHT_CASOLVEROUTPUTANALYZER_H

#include "base/outputanalyzer.h"
#include "base/progressdisplayer.h"

#include <map>
#include <memory>
#include <armadillo>

#include "base/boost_include.h"
#include "boost/regex.hpp"

namespace insight {

struct Message : public std::string {
  enum MsgType {None, Warning, Exception} msgType_ = None;
  std::string msgCode_;
};

std::ostream &operator<<(std::ostream& os, const Message& msg);


class CASolverOutputAnalyzer
    : public OutputAnalyzer
{
  boost::regex re_excep, re_alarm, re_msgstr, re_msgdelim, re_instant;

  double endTime_;

  std::shared_ptr<ActionProgress> solverActionProgress_;

  std::unique_ptr<Message> currentMessage_;

  std::vector<std::shared_ptr<Message> > messages_;

public:
  CASolverOutputAnalyzer ( ProgressDisplayer& pd, double endTime=1000 );

  void update (const std::string& line) override;

  bool stopRun() const override;

  std::vector<std::shared_ptr<Message> > messages(Message::MsgType t) const;
  std::vector<std::shared_ptr<Message> > warnings() const;
  std::vector<std::shared_ptr<Message> > exceptions() const;
};



} // namespace insight

#endif // INSIGHT_CASOLVEROUTPUTANALYZER_H
