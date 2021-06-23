#ifndef WSLRUN_H
#define WSLRUN_H

#include <memory>

#include "remoterun.h"
#include "analyzeclient.h"
#include "base/remoteexecution.h"

#include "qanalysisthread.h"

class WSLRun
    : public RemoteRun
{
  Q_OBJECT

  boost::filesystem::path WSLExecutable_;

  void launchRemoteAnalysisServer() override;

  WSLRun(AnalysisForm* af, const boost::filesystem::path& WSLExecutable, bool resume=false);

public:
  static std::unique_ptr<WSLRun> create(AnalysisForm* af, const boost::filesystem::path& WSLExecutable, bool resume=false);

};


#endif // WSLRUN_H
