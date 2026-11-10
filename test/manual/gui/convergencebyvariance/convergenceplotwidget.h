#ifndef CONVERGENCEPLOTWIDGET_H
#define CONVERGENCEPLOTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QRubberBand>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>

#include "evaluate.h"


/**
 * Chart view, in which a time window can be selected by dragging
 * with the left mouse button. Double click resets the window.
 */
class TimeWindowChartView : public QtCharts::QChartView
{
  Q_OBJECT

  QRubberBand *rubberBand_;
  QPoint origin_;
  bool dragging_ = false;

  double xValue(const QPoint& p) const;

public:
  TimeWindowChartView(QtCharts::QChart* chart, QWidget* parent=nullptr);

protected:
  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;
  void mouseDoubleClickEvent(QMouseEvent* e) override;

Q_SIGNALS:
  void timeWindowSelected(double t0, double t1);
  void timeWindowReset();
};


/**
 * Displays one data column: raw signal with the evaluation window
 * highlighted (top) and the convergence measure history (bottom).
 */
class ConvergencePlotWidget : public QWidget
{
  Q_OBJECT

  arma::mat tx_; // time, value

  QtCharts::QChart *signalChart_, *covChart_;
  QtCharts::QLineSeries *raw_, *selected_, *startMarker_, *endMarker_, *cov_, *threshold_;
  QtCharts::QValueAxis *sx_, *sy_, *cx_;
  QtCharts::QLogValueAxis *cy_;
  QtCharts::QLineSeries *convergedMarker_;
  double convergedTime_;
  QLabel *info_;

  void updateConvergedMarker();

public:
  ConvergencePlotWidget(QWidget* parent=nullptr);

  /**
   * set the displayed series (n x 2: time, value)
   */
  void setSeries(const arma::mat& tx, const QString& title);

  void showResult(const ColumnResult& r, const Criterion& crit);

  /**
   * mark time, at which all selected series of the case
   * were converged for the first time (NaN: never)
   */
  void setConvergedTime(double t);

Q_SIGNALS:
  void timeWindowSelected(double t0, double t1);
  void timeWindowReset();
};

#endif
