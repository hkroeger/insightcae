#ifndef TASKSPOOLERMONITOR_H
#define TASKSPOOLERMONITOR_H

#include <QWidget>
#include <QDialog>
#include <QProcess>

#include "base/boost_include.h"

namespace Ui {
class TaskSpoolerMonitor;
}




class TaskSpoolerMonitor
: public QWidget
{
  Q_OBJECT

  boost::filesystem::path tsp_socket_;
  QProcessEnvironment env_;

public:
  explicit TaskSpoolerMonitor(const boost::filesystem::path& tsp_socket, QWidget *parent = nullptr);
  ~TaskSpoolerMonitor();

public slots:
  void onRefresh();
  void onClean();
  void onKill();
  void onStartTail();
  void onFinishedTail(int exitcode, QProcess::ExitStatus exitStatus);

  void onOutputReady();
  void onErrorReady();

private:
  Ui::TaskSpoolerMonitor *ui;
};




class TaskSpoolerMonitorDialog
: public QDialog
{
public:
    TaskSpoolerMonitorDialog(const boost::filesystem::path& tsp_socket, QWidget *parent = nullptr);
};

#endif // TASKSPOOLERMONITOR_H
