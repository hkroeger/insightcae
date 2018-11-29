/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


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
