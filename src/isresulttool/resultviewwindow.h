#ifndef RESULTVIEWWINDOW_H
#define RESULTVIEWWINDOW_H

#include <QMainWindow>

#include "iqresultsetmodel.h"
#include "iqresultsetfiltermodel.h"

namespace Ui {
class ResultViewWindow;
}

class ResultViewWindow : public QMainWindow
{
  Q_OBJECT

  insight::IQResultSetModel *resultsModel_;
  insight::IQFilteredResultSetModel *filteredResultsModel_;
  IQResultSetFilterModel *filterModel_;

public:
  explicit ResultViewWindow(QWidget *parent = nullptr);
  ~ResultViewWindow();

  void loadResults(insight::ResultSetPtr results);

private:
  Ui::ResultViewWindow *ui;
};

#endif // RESULTVIEWWINDOW_H
