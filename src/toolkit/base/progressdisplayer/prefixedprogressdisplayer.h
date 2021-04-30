#ifndef PREFIXEDPROGRESSDISPLAYER_H
#define PREFIXEDPROGRESSDISPLAYER_H


#include "base/progressdisplayer.h"

namespace insight
{

class PrefixedProgressDisplayer
    : public ProgressDisplayer
{
public:
  enum ProgressVariablePrefixType { NoProgressVariablePrefix, Prefixed};
  enum ActionProgressPrefixType { NoActionProgressPrefix, HierarchyLevelPrefix, ParallelPrefix};

protected:
  ProgressDisplayer* parent_;
  std::string prefix_;
  ProgressVariablePrefixType pvPrefix_;
  ActionProgressPrefixType actionPrefix_;

  std::string prefixedPVPath(const std::string& path) const;

public:
  PrefixedProgressDisplayer(ProgressDisplayer* parent, const std::string& prefix,
                            ProgressVariablePrefixType progressVariablePrefix = Prefixed,
                            ActionProgressPrefixType actionPrefix = NoActionProgressPrefix);

  void setActionProgressValue(const std::string &path, double value) override;
  void setMessageText(const std::string &path, const std::string& message) override;
  void finishActionProgress(const std::string &path) override;

  void update ( const ProgressState& pi ) override;
  void reset() override;

  bool stopRun() const override;
};

}

#endif // PREFIXEDPROGRESSDISPLAYER_H
