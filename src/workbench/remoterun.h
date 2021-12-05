#ifndef REMOTERUN_H
#define REMOTERUN_H

#include <memory>

#include "workbenchaction.h"
#include "analyzeclient.h"
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
  insight::RemoteExecutionConfig& remote_;
  std::unique_ptr<insight::AnalyzeClient> ac_;
  insight::RemoteServer::BackgroundJobPtr analyzeProcess_;
  bool cancelled_, disconnected_;
  insight::ActionProgress launchProgress_;


protected:
  RemoteRun(AnalysisForm* af, insight::RemoteExecutionConfig& rec, bool resume=false);

  void launch();

  // STEPS:
  // 1. check remote work dir, launch machine and create, if needed
  void setupRemoteEnvironment();
  void undoSetupRemoteEnvironment();

  // 2. launch remote execution server
  void launchRemoteExecutionServer();
  void undoLaunchRemoteExecutionServer();

  // 3. contact remote exec server
  void waitForContact( int maxAttempt=20 );

  // 4. launch analysis
  void launchAnalysis();

  // 5. monitor running simulation
  void monitor();

  // 6. fetch results, trigger server exit
  void fetchResults();

  // 7. stop remote server
  void stopRemoteExecutionServer();

  // 8. cleanup remote
  void cleanupRemote();

  bool checkIfCancelled();
  void onErrorString(const std::string& errorMessage);
  void onError(std::exception_ptr ex);

public:
  static std::unique_ptr<RemoteRun> create(AnalysisForm* af, insight::RemoteExecutionConfig& rec, bool resume=false);

  ~RemoteRun();

  inline insight::AnalyzeClient& analyzeClient() { return *ac_; }

public Q_SLOTS:
  void onCancel() override;
};

#endif // REMOTERUN_H
