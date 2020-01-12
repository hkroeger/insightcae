#ifndef PREFIXEDPROGRESSDISPLAYER_H
#define PREFIXEDPROGRESSDISPLAYER_H


#include "base/progressdisplayer.h"

namespace insight
{

class PrefixedProgressDisplayer
    : public ProgressDisplayer
{
  ProgressDisplayer* parent_;
  std::string prefix_;

public:
  PrefixedProgressDisplayer(ProgressDisplayer* parent, const std::string& prefix);

  virtual void update ( const ProgressState& pi );
  virtual bool stopRun() const;
};

}

#endif // PREFIXEDPROGRESSDISPLAYER_H
