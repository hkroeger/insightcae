#ifndef RESULTVIEWWINDOW_H
#define RESULTVIEWWINDOW_H

#include <QMainWindow>

#include "iqresultsetdisplayerwidget.h"

namespace Ui {
class ResultViewWindow;
}

class ResultViewWindow : public QMainWindow
{
  Q_OBJECT

  IQResultSetDisplayerWidget *viewer_;

public:
  explicit ResultViewWindow(QWidget *parent = nullptr);
  ~ResultViewWindow();

  void loadResults(insight::ResultSetPtr results);

private:
  Ui::ResultViewWindow *ui;
};

#endif // RESULTVIEWWINDOW_H
