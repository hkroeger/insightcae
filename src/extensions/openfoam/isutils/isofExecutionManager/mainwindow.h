#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

protected:
    void updateGUI();

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

public Q_SLOTS:
    void onSelectRemoteDir();
    void syncLocalToRemote();
    void syncRemoteToLocal();

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
