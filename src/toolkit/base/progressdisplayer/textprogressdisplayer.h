#ifndef TEXTPROGRESSDISPLAYER_H
#define TEXTPROGRESSDISPLAYER_H

#include "base/progressdisplayer.h"

namespace insight
{

class TextProgressDisplayer
    : public ProgressDisplayer
{
public:
  void update ( const ProgressState& pi ) override;
  void setActionProgressValue(const std::string& path, double value) override;
  void setMessageText(const std::string& path, const std::string& message) override;
  void finishActionProgress(const std::string& path) override;
  void reset() override;
};


extern TextProgressDisplayer consoleProgressDisplayer;

}

#endif // TEXTPROGRESSDISPLAYER_H
