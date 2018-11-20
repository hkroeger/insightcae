#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include <QThread>

#include "base/linearalgebra.h"

#include "qwt_scale_engine.h"
#include "qwt_plot_grid.h"
#include "qwt_legend.h"
#include "qwt_plot_curve.h"

namespace Ui {
  class PlotWidget;
}

Q_DECLARE_METATYPE(arma::mat);

class MeanComputer : public QThread
{
  Q_OBJECT
  const arma::mat& rawdata_;
  double frac_;
public:
  MeanComputer(QObject *parent, const arma::mat& rawdata, double frac);
  virtual void run();
Q_SIGNALS:
  void resultReady(arma::mat meandata);
};


class PlotWidget : public QWidget
{
  Q_OBJECT

  arma::mat rawdata_;
  MeanComputer *mc_=NULL;

public:
  explicit PlotWidget(QWidget *parent = 0);
  ~PlotWidget();

  void setData(const arma::mat& x, const arma::mat& y);

public Q_SLOT:
  void onShow();
  void onMeanAvgFractionChange(double x=0);
  void onMeanDataReady(arma::mat meandata);
  void onChangeX0(double x0);
  void onToggleY0(bool);

private:
  Ui::PlotWidget *ui;
  QwtPlot* plot_;
  QwtPlotCurve *raw_crv_, *mean_crv_;
};

#endif // PLOTWIDGET_H
