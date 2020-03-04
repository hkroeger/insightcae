#ifndef REMOTERUN_H
#define REMOTERUN_H

#include <memory>

#include "workbenchaction.h"
#include "analyzeclient.h"
#include "openfoam/remoteexecution.h"


class RemoteRun
    : public WorkbenchAction
{
  Q_OBJECT

  bool resume_;
  boost::thread workerThread_, cancelThread_;
  std::unique_ptr<insight::RemoteExecutionConfig> remote_;
  insight::AnalyzeClient ac_;

  void launchRemoteAnalysisServer();

public:
  RemoteRun(AnalysisForm* af, bool resume=false);
  ~RemoteRun();

  insight::RemoteExecutionConfig& remote();

public Q_SLOTS:
  void onCancel() override;
};

#endif // REMOTERUN_H
