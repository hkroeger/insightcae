#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>

#include "base/linearalgebra.h"

#include "qwt_scale_engine.h"
#include "qwt_plot_grid.h"
#include "qwt_legend.h"
#include "qwt_plot_curve.h"

namespace Ui {
  class PlotWidget;
}

class PlotWidget : public QWidget
{
  Q_OBJECT

public:
  explicit PlotWidget(QWidget *parent = 0);
  ~PlotWidget();

  void setData(const arma::mat& x, const arma::mat& y);

private:
  Ui::PlotWidget *ui;
  QwtPlot* plot_;
  QwtPlotCurve *raw_crv_, *mean_crv_;
};

#endif // PLOTWIDGET_H
