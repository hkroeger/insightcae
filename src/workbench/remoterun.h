#ifndef REMOTERUN_H
#define REMOTERUN_H

#include <memory>

#include "workbenchaction.h"
#include "analyzeclient.h"
#include "base/remoteexecution.h"

#include "qanalysisthread.h"




class ExecutionProgram
{
public:
  typedef std::function<void()> RunFunction;
  typedef std::function<void()> UndoFunction;
  typedef std::pair< RunFunction, UndoFunction > Step;
  typedef std::vector<Step> StepList;

protected:
  StepList steps_;

public:
  ExecutionProgram(const StepList& steps);
  void run();
};




class RemoteRun
    : public WorkbenchAction
{
  Q_OBJECT

  bool resume_;
  insight::RemoteServer::PortMappingPtr portMappings_;
  insight::RemoteExecutionConfig& remote_;
  boost::thread cancelThread_;
  std::unique_ptr<insight::AnalyzeClient> ac_;
  insight::RemoteServer::BackgroundJobPtr analyzeProcess_;
  int remotePid_ =-1;
  std::unique_ptr<insight::QAnalysisThread> workerThread_;

  virtual void launchRemoteAnalysisServer();


protected:
  RemoteRun(AnalysisForm* af, insight::RemoteExecutionConfig& rec, bool resume=false);

  void launch();

public:
  static std::unique_ptr<RemoteRun> create(AnalysisForm* af, insight::RemoteExecutionConfig& rec, bool resume=false);

  ~RemoteRun();


public Q_SLOTS:
  void onCancel() override;
};

#endif // REMOTERUN_H
