#ifndef REMOTERUN_H
#define REMOTERUN_H

#include <memory>

#include "workbenchaction.h"
#include "analyzeclient.h"
#include "base/remoteexecution.h"

#include "qanalysisthread.h"

class RemoteRun
    : public WorkbenchAction
{
  Q_OBJECT

  bool resume_;
  insight::RemoteExecutionConfig& remote_;
  boost::thread cancelThread_;
  std::unique_ptr<insight::AnalyzeClient> ac_;
  std::unique_ptr<insight::QAnalysisThread> workerThread_;

  void launchRemoteAnalysisServer();

public:
  RemoteRun(AnalysisForm* af, bool resume=false);
  ~RemoteRun();


public Q_SLOTS:
  void onCancel() override;
};

#endif // REMOTERUN_H
