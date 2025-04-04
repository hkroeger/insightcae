#ifndef REMOTERUN_H
#define REMOTERUN_H

#include <memory>

#include "workbenchaction.h"
#include "analyzeclient.h"
#include "iqexecutionworkspace.h"
#include "base/remoteexecution.h"

#include "qanalysisthread.h"




class UndoSteps
{
public:
  typedef std::function<void()> UndoFunction;
  struct UndoStep
  {
      UndoFunction undoFunction;
      std::string label;
  };
  typedef std::vector<UndoStep> UndoStepList;

protected:
  UndoStepList undoSteps_;


public:
  void addUndoStep(UndoFunction undoFunction, const std::string& label);
  void performUndo(std::exception_ptr undoReason, bool rethrow = false);
};




class RemoteRun
    : public WorkbenchAction,
      protected UndoSteps
{
  Q_OBJECT

  bool resume_;
  insight::RemoteServer::PortMappingPtr portMappings_;
  IQRemoteExecutionState* remote_;
  std::unique_ptr<insight::AnalyzeClient> ac_;
  insight::RemoteServer::BackgroundJobPtr analyzeProcess_;
  bool killRequested_, disconnectRequested_;
  insight::ActionProgress launchProgress_;

  insight::ResultSetPtr results_;


protected:
  RemoteRun(AnalysisForm* af, bool resume=false);

  void launch();

  // STEPS:
  // 1. check remote work dir, launch machine and create, if needed
  void setupRemoteEnvironment();
  void undoSetupRemoteEnvironment();

  // 2. upload input file
  void uploadInputFile();

  // 3. launch remote execution server
  void launchRemoteExecutionServer();
  void undoLaunchRemoteExecutionServer();

  // 4. contact remote exec server
  void waitForContact( int maxAttempt=20 );

  // // 5. launch analysis
  // void launchAnalysis();

  // 5. monitor running simulation
  void monitor();

  // 6. fetch results, trigger server exit
  void fetchResults();

  // 7. stop remote server
  void stopRemoteExecutionServer();

  // 8. download
  void download();

  // 9. cleanup remote
  void cleanupRemote();

  // 10. finish
  void finish();

  void checkIfCancelled();
  void onErrorString(const std::string& errorMessage);
  void onError(std::exception_ptr ex);

public:
  static RemoteRun* create(AnalysisForm* af, bool resume=false);

  ~RemoteRun();

  inline insight::AnalyzeClient& analyzeClient() { return *ac_; }

public Q_SLOTS:
  void onCancel() override;
};

#endif // REMOTERUN_H
