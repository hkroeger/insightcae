#ifndef TASKSPOOLERMONITOR_H
#define TASKSPOOLERMONITOR_H

#include <QWidget>
#include <QDialog>
#include <QProcess>

#include "base/boost_include.h"
#include "openfoam/remoteexecution.h"

namespace Ui {
class TaskSpoolerMonitor;
}




class TaskSpoolerMonitor
: public QWidget,
  public insight::TaskSpoolerInterface
{
  Q_OBJECT

public:
  explicit TaskSpoolerMonitor(const boost::filesystem::path& tsp_socket, const QString& remote_machine="", QWidget *parent = nullptr);
  ~TaskSpoolerMonitor();

public Q_SLOTS:
  void onRefresh();
  void onClean();
  void onKill();
  void onStartTail();
  void onFinishedTail(int exitcode, QProcess::ExitStatus exitStatus);

//  void onOutputReady(const std::string& line);
//  void onErrorReady();

Q_SIGNALS:
  void outputReady(const QString& line);

private:
  Ui::TaskSpoolerMonitor *ui;
};




class TaskSpoolerMonitorDialog
: public QDialog
{
public:
    TaskSpoolerMonitorDialog(const boost::filesystem::path& tsp_socket, const QString& remote_machine="", QWidget *parent = nullptr);
};

#endif // TASKSPOOLERMONITOR_H
