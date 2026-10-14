#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QProgressBar>
#include <QPointer>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileSystemModel>

#include "boost/optional.hpp"

#ifndef NO_TERMWIDGET
#include "qtermwidget/qtermwidget.h"
#endif
#include "base/boost_include.h"
#include "base/remoteexecution.h"
#include "openfoam/openfoamcase.h"
#include "iqremoteexecutionstate.h"
#include "remotedirselector.h"
#include "remotesync.h"
#include "iqremotefoldermodel.h"

namespace Ui {
  class MainWindow;
}



class JobListBuilder
    : public QThread
{
  Q_OBJECT

  std::shared_ptr<insight::TaskSpoolerInterface> tsi_;

public:
  JobListBuilder(std::shared_ptr<insight::TaskSpoolerInterface> tsi);

  void run() override;

Q_SIGNALS:
  void jobListReady(insight::TaskSpoolerInterface::JobList jl);

};


class AuxiliaryJob
    : public QObject
{
  Q_OBJECT

  insight::JobPtr job_;

public:
  AuxiliaryJob(insight::JobPtr job);

  void run();

Q_SIGNALS:
  void outputLineReceived(const QString& line);
  void completed(int returnValue);
};



class IQRXRemoteExecutionState
    : public IQRemoteExecutionState
{
protected:
  void updateGUI(bool enabled) override;
};


class MainWindow
: public QMainWindow
{
  Q_OBJECT

  friend class IQRXRemoteExecutionState;

  QPointer<IQRemoteExecutionState> remote_;

#ifndef NO_TERMWIDGET
  QTermWidget *terminal_;
#endif

  std::shared_ptr<insight::TaskSpoolerInterface> tsi_;
  std::shared_ptr<insight::OutputAnalyzer> soa_;

  QTimer *refreshTimer_;
  JobListBuilder* jbl_thread_ = nullptr;

//  QHBoxLayout* statusBarLayout_;
//  QProgressBar* progressbar_;

  QThread auxJobThread_;

  int bwLimit_ = -1; // applied, when connected

  // state of the remote host (determined in background)
  QLabel *serverStatusIcon_, *serverStatus_;
  QPushButton *btnLaunch_, *btnShutdown_, *btnDispose_, *btnRefreshStatus_;
  boost::optional<bool> serverOnline_, serverExists_;
  boost::optional<insight::RemoteServer::Config::ProcessorLoad> serverLoad_;
  int statusGeneration_ = 0; // to discard results of outdated checks

  // file trees in the status tab
  QFileSystemModel *localFilesModel_;
  IQRemoteFolderModel *remoteFilesModel_ = nullptr;
  insight::RemoteServerPtr remoteFilesServer_;
  boost::filesystem::path remoteFilesDir_;

  /**
   * @brief setRemoteFilesView
   * display the contents of the remote directory in the remote file tree
   * @param server
   * the connected server. If null, the tree is cleared.
   */
  void setRemoteFilesView(
      insight::RemoteServerPtr server,
      const boost::filesystem::path& remoteDir = boost::filesystem::path() );

  /**
   * @brief localSelection
   * @return
   * the files and directories selected in the local file tree,
   * relative to the local directory
   */
  std::vector<boost::filesystem::path> localSelection() const;

  /**
   * @brief remoteSelection
   * @return
   * the files and directories selected in the remote file tree,
   * relative to the remote directory
   */
  std::vector<boost::filesystem::path> remoteSelection() const;

  /**
   * @brief ensureConnected
   * connect to the remote location, if not yet done
   * (launching the host, if required and possible). Displays errors.
   * @return
   * true, if connected
   */
  bool ensureConnected();

  /**
   * @brief checkServerStatus
   * determine the state of the remote host in background
   */
  void checkServerStatus();
  void updateServerControls();

  void launchServer();
  void shutdownServer();
  void disposeServer();
  void removeRemoteDirectory();

  /**
   * @brief setRemote
   * replace the remote location state
   */
  void setRemote(IQRemoteExecutionState* newRemote);

protected:
//    void updateGUI();
    void onStartTail();

Q_SIGNALS:
    void logReady(QString line);

public:
  explicit MainWindow(const boost::filesystem::path& location, QWidget *parent = nullptr);
  ~MainWindow();

    virtual void closeEvent(QCloseEvent *event);
    void saveSettings();
    void readSettings();

    void remoteWriteAndCopyBack(bool parallel);

    /**
     * @param selection
     * items to transfer (relative paths). If empty, the complete directory is synchronized.
     */
    void syncLocalToRemote(
        bool includeProcDirs,
        const std::vector<boost::filesystem::path>& selection = {} );
    void syncRemoteToLocal(
        bool includeProcDirs, int autoRestartSeconds=-1,
        const std::vector<boost::filesystem::path>& selection = {} );
    void setBWLimit(int bwlimit);

public Q_SLOTS:
    void onStartParaview();
    void onStartRemoteParaview();
    void onClearProgressCharts();

    void updateOutputAnalzer(QString line);

    void onRefreshJobList();
    void onJobListReady(insight::TaskSpoolerInterface::JobList jl);

private:
  Ui::MainWindow *ui;

};








#endif // MAINWINDOW_H
