#ifndef CASEWIDGET_H
#define CASEWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>

#include "casereader.h"
#include "evaluate.h"

class ConvergencePlotWidget;

/**
 * Displays one case: list of all series (checkable, colored by
 * convergence status), case specific time window
 * and a plot of the selected series.
 */
class CaseWidget : public QWidget
{
  Q_OBJECT

  CaseData& case_;
  std::vector<ColumnResult> results_;
  Criterion crit_;

  QLineEdit *startTime_, *endTime_;
  QListWidget *list_;
  ConvergencePlotWidget *plot_;

  void onCurrentRowChanged(int row);
  void onItemChanged(QListWidgetItem* item);
  void updateItems();

public:
  static const QColor convergedColor, convergedEarlierColor, notConvergedColor;

  CaseWidget(CaseData& cd, QWidget* parent=nullptr);

  void setResults(const std::vector<ColumnResult>& results);
  void setCriterion(const Criterion& c);

  /**
   * case specific time window, NaN if not set
   */
  void timeWindow(double& t0, double& t1) const;

  CaseStatus status() const;
  bool hasResults() const;

public Q_SLOTS:
  void setTimeWindow(double t0, double t1);
  void resetTimeWindow();

Q_SIGNALS:
  /** the case specific time window was changed => re-evaluate */
  void timeWindowChanged();
  /** a series was (de-)selected => update status, store selection */
  void selectionChanged();
};

#endif
