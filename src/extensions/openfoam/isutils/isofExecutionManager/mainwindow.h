#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "terminal.h"

#include "base/boost_include.h"
#include "openfoam/remoteexecution.h"
#include "openfoam/openfoamcase.h"


namespace Ui {
  class MainWindow;
}

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

protected:
    void updateGUI();
    void onRefreshJobList();
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

private:
  Ui::MainWindow *ui;

  QString rem_subdir_, rem_host_;
};

#endif // MAINWINDOW_H
