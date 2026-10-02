#ifndef REMOTERUN_H
#define REMOTERUN_H

#include <memory>
#include <QPointer>
#include <atomic>
#include <chrono>

#include "base/resultset.h"
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
    : public QObject,
      public WorkbenchAction,
      protected UndoSteps
{
  Q_OBJECT

public:
  /**
   * @brief The Timing struct
   * intervals and limits used during a remote run
   */
  struct Timing
  {
      int contactAttempts = 20;
      std::chrono::milliseconds contactInterval {2000};
      std::chrono::milliseconds pollInterval {1000};
      std::chrono::milliseconds requestTimeout {15*60*1000};
      int maxFailedStatusQueries = 5; // consecutive failed status queries until contact is considered lost
      std::chrono::milliseconds cancelRequestTimeout {10000}; // for the kill/exit requests upon cancellation
  };

  /**
   * @brief defaultTiming
   * timing, which is used by subsequently created remote runs
   */
  static Timing& defaultTiming();

private:
  Timing timing_;
  bool resume_;
  insight::RemoteServer::PortMappingPtr portMappings_;
  QPointer<IQRemoteExecutionState> remote_; // GUI object: only to be used in the GUI thread
  std::unique_ptr<insight::RemoteExecutionConfig> rec_; // copy for the work in the IO threads
  bool downloadWhenFinished_ = false;
  std::unique_ptr<insight::ParameterSet> inputParameters_; // copied in the GUI thread
  std::unique_ptr<insight::AnalyzeClient> ac_;
  insight::RemoteServer::BackgroundJobPtr analyzeProcess_;
  std::atomic<bool> killRequested_, disconnectRequested_;
  std::atomic<bool> ended_; // set by the first of finish, failure and cancellation
  std::atomic<bool> contactEstablished_; // the remote analyze server has answered
  int failedStatusQueries_ = 0;
  insight::ActionProgressPtr launchProgress_;

  std::unique_ptr<insight::ResultSet> results_;


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
  void waitForContact( int maxAttempt );

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

  /**
   * @brief claimEnd
   * @return true for the first caller only: the run ends with this caller's outcome
   */
  bool claimEnd();

  // cancellation (IO thread):
  // ask the remote analyze server to stop, then roll back
  void cancelRemoteRun();
  void completeCancellation(bool remoteServerStopped);

  /**
   * @brief cleanupRemoteLocation
   * remove the temporary remote location (IO thread) and
   * discard the remote state in the GUI (posted to the GUI thread)
   */
  void cleanupRemoteLocation();

  /**
   * @brief remoteAnalyzeLog
   * last lines of the remote analyze log, formatted for an error message (empty, if not available)
   */
  std::string remoteAnalyzeLog();

  /**
   * @brief remoteAnalyzeHasExited
   * true, if the remote analyze process, which was launched by this run, is not running anymore
   */
  bool remoteAnalyzeHasExited();
  void onErrorString(const std::string& errorMessage);
  void onError(std::exception_ptr ex);

public:
  static RemoteRun* create(AnalysisForm* af, bool resume=false);

  ~RemoteRun();

  std::unique_ptr<insight::ResultSet> moveResults() override;

  inline insight::AnalyzeClient& analyzeClient() { return *ac_; }

public Q_SLOTS:
  void onCancel() override;

Q_SIGNALS:
  void logMessage(const QString& logmsg);
  void statusMessage(const QString& msg);

  void finished();
  void failed(::std::exception_ptr e);
  void cancelled();
};

#endif // REMOTERUN_H
