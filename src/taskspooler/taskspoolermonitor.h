#ifndef TASKSPOOLERMONITOR_H
#define TASKSPOOLERMONITOR_H

#include <QWidget>
#include <QDialog>

#include "base/boost_include.h"

namespace Ui {
class TaskSpoolerMonitor;
}

class TaskSpoolerMonitor
: public QWidget
{
  Q_OBJECT

  boost::filesystem::path tsp_socket_;

public:
  explicit TaskSpoolerMonitor(const boost::filesystem::path& tsp_socket, QWidget *parent = nullptr);
  ~TaskSpoolerMonitor();

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
