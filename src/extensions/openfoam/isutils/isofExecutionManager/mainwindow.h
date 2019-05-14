#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "terminal.h"

#include "base/boost_include.h"
#include "openfoam/remoteexecution.h"


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

protected:
    void updateGUI();
    void onRefreshJobList();
    void onStartTail();

Q_SIGNALS:
    void logReady(const QString& line);

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
    void onStartRemoteParaviewSubdir();

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
