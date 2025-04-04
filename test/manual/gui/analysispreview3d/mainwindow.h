#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifndef Q_MOC_RUN
#include "base/analysis.h"
#include "base/resultset.h"
#include "base/remoteexecution.h"
#include "parametereditorwidget.h"
#endif

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

protected:

  // ====================================================================================
  // ======== Analysis-related members
  std::unique_ptr<insight::ParameterSet> parameters_;

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
