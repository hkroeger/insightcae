#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QProgressBar>

#include "terminal.h"
#include "base/boost_include.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/remoteexecution.h"
#include "remotedirselector.h"
#include "remotesync.h"

namespace Ui {
  class MainWindow;
}


Q_DECLARE_METATYPE(insight::TaskSpoolerInterface::JobList)

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


class MainWindow
: public QMainWindow,
  public insight::RemoteExecutionConfig
{
  Q_OBJECT

#ifdef HAVE_KF5
  TerminalWidget *terminal_;
#endif
  std::shared_ptr<insight::TaskSpoolerInterface> tsi_;
  std::shared_ptr<insight::SolverOutputAnalyzer> soa_;

  QTimer *refreshTimer_;
  JobListBuilder* jbl_thread_ = nullptr;

  QProgressBar* progressbar_;

protected:
    void updateGUI();
    void onStartTail();

Q_SIGNALS:
    void logReady(QString line);

public:
  explicit MainWindow(const boost::filesystem::path& location, QWidget *parent = nullptr);
  ~MainWindow();

    virtual void closeEvent(QCloseEvent *event);
    void saveSettings();
    void readSettings();

public Q_SLOTS:
    void onSelectRemoteDir();
    void syncLocalToRemote();
    void syncRemoteToLocal();

    void onStartParaview();
    void onStartRemoteParaview();
    void onClearProgressCharts();

    void updateOutputAnalzer(QString line);

    void onRefreshJobList();
    void onJobListReady(insight::TaskSpoolerInterface::JobList jl);

private:
  Ui::MainWindow *ui;

  QString rem_subdir_, rem_host_;
};








#endif // MAINWINDOW_H
