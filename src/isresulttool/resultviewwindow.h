#ifndef RESULTVIEWWINDOW_H
#define RESULTVIEWWINDOW_H

#include <QDialog>

#include "qresultsetmodel.h"

namespace Ui {
class ResultViewWindow;
}

class ResultViewWindow : public QDialog
{
  Q_OBJECT

  insight::QResultSetModel resultsModel_;

public:
  explicit ResultViewWindow(insight::ResultSetPtr results, QWidget *parent = nullptr);
  ~ResultViewWindow();

private:
  Ui::ResultViewWindow *ui;
};

#endif // RESULTVIEWWINDOW_H
