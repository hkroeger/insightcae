#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QProgressBar>

#include "qtermwidget/qtermwidget.h"
#include "base/boost_include.h"
#include "base/remoteexecution.h"
#include "openfoam/openfoamcase.h"
#include "remotedirselector.h"
#include "remotesync.h"

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



class MainWindow
: public QMainWindow
{
  Q_OBJECT

  std::unique_ptr<insight::RemoteExecutionConfig> remote_;

  QTermWidget *terminal_;

  std::shared_ptr<insight::TaskSpoolerInterface> tsi_;
  std::shared_ptr<insight::OutputAnalyzer> soa_;

  QTimer *refreshTimer_;
  JobListBuilder* jbl_thread_ = nullptr;

  QProgressBar* progressbar_;

  QThread auxJobThread_;

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

    void remoteWriteAndCopyBack(bool parallel);

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

};








#endif // MAINWINDOW_H
