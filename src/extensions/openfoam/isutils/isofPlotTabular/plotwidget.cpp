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

#include "plotwidget.h"
#include "base/exception.h"
#include "ui_plotwidget.h"


#include "base/boost_include.h"

#include "base/cppextensions.h""
#include "base/qt5_helper.h"

#include <QtCharts/QValueAxis>

using namespace std;
using namespace boost;

PlotWidget::PlotWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::PlotWidget)
{
  ui->setupUi(this);

  QVBoxLayout *layout=new QVBoxLayout(ui->plotcanvas);
  ui->plotcanvas->setLayout(layout);

  plotData_=new QtCharts::QChart();

  plot_=new /*QwtPlot*/QtCharts::QChartView(plotData_, this);
  layout->addWidget(plot_);


  plotData_->setBackgroundBrush( Qt::white );
  plotData_->addAxis(new QtCharts::QValueAxis, Qt::AlignLeft);
  plotData_->addAxis(new QtCharts::QValueAxis, Qt::AlignBottom);

  plotData_->axes(Qt::Horizontal)[0]->setGridLineVisible(true);
  plotData_->axes(Qt::Vertical)[0]->setGridLineVisible(true);


  raw_crv_=new QtCharts::QLineSeries();
  raw_crv_->setName("raw");
  raw_crv_->setPen(QPen(Qt::red, 2.0));
  plotData_->addSeries(raw_crv_);
  raw_crv_->attachAxis(plotData_->axes(Qt::Horizontal)[0]);
  raw_crv_->attachAxis(plotData_->axes(Qt::Vertical)[0]);


  mean_crv_=new QtCharts::QLineSeries();
  mean_crv_->setName("mean");
  mean_crv_->setPen(QPen(Qt::black, 3.0));
  plotData_->addSeries(mean_crv_);
  mean_crv_->attachAxis(plotData_->axes(Qt::Horizontal)[0]);
  mean_crv_->attachAxis(plotData_->axes(Qt::Vertical)[0]);

  connect(ui->include_0_sw, &QCheckBox::toggled,
          this, &PlotWidget::onToggleY0);

  connect(ui->avg_fraction, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &PlotWidget::onMeanAvgFractionChange);

}

PlotWidget::~PlotWidget()
{
  if (mc_)
  {
    if (mc_->isRunning()) mc_->terminate();
    delete mc_;
  }
  delete ui;
}

void PlotWidget::setData(const arma::mat& x, const arma::mat& y)
{
  rawdata_=arma::join_horiz(x,y);
  visible_part_=rawdata_;

  raw_crv_->clear();
  for (arma::uword i=0; i< x.n_rows; i++)
    raw_crv_->append(x(i), y(i));

  mean_crv_->clear();


  ui->final_vals->setText(QString::fromStdString(
    str(format("Final values: raw=%g / mean = (-)")%y(y.n_rows-1))
  ));

  onToggleY0(true);
}

void PlotWidget::changeAvgFraction(double f)
{
    ui->avg_fraction->setValue(f);
}

void PlotWidget::onShow()
{
  if (mean_crv_->count()==0)
  {
    onMeanAvgFractionChange();
  }
}

void PlotWidget::onMeanAvgFractionChange(double)
{
  if (!mc_)
  {
    ui->info->setText("Computing moving average of raw data...");
    mc_=new MeanComputer( this, visible_part_, ui->avg_fraction->value() );
    connect(mc_, &MeanComputer::resultReady, this, &PlotWidget::onMeanDataReady);
    mc_->start();
  }
}

MeanComputer::MeanComputer(QObject* p, const arma::mat& rd, double frac)
  : QThread(p),
    rawdata_(rd),
    frac_(frac)
{}

void MeanComputer::run()
{
  emit resultReady( insight::movingAverage(rawdata_, frac_) );
}

double MeanComputer::fraction() const
{
    return frac_;
}

void PlotWidget::onMeanDataReady(arma::mat avg)
{
  if (avg.n_rows>0)
  {
    ui->final_vals->setText(QString::fromStdString(
      str(format("Final values: raw=%g / mean = %g")
           %( rawdata_.col(1)(rawdata_.n_rows-1) )
           %( avg.col(1)(avg.n_rows-1) ))
    ));

    mean_crv_->clear();
    for (arma::uword i=0; i<avg.n_rows; ++i)
      mean_crv_->append(avg(i,0), avg(i,1));

    ui->info->setText("Moving average computed.");
  //  plot_->replot();
  }
  else
  {
    ui->info->setText("Empty moving average.");
  }

  mc_->wait();

  bool startOver=false;
  if (mc_->fraction()!=ui->avg_fraction->value())
      startOver=true;

  delete mc_;
  mc_=nullptr;

  if (startOver)
      onMeanAvgFractionChange();
  else
      Q_EMIT averageValueReady();
}


void PlotWidget::onChangeXRange(const QString& v0, const QString& v1)
{
  double x0=rawdata_.col(0).min();
  double x1=rawdata_.col(0).max();

  if (!v0.isEmpty()) x0=v0.toDouble();
  if (!v1.isEmpty()) x1=v1.toDouble();

  visible_part_=rawdata_.rows( arma::find(rawdata_.col(0)>x0 && rawdata_.col(0)<x1) );
  double ymin=0., ymax=1.;
  if (visible_part_.n_rows>0)
  {
    ymin=visible_part_.col(1).min();
    ymax=visible_part_.col(1).max();

    if (ui->include_0_sw->isChecked())
    {
      if (ymin>0.) ymin=0.;
      if (ymax<0.) ymax=0.;
    }

    if (ymin<0.) ymin*=1.1; else ymin/=1.1;
    if (ymax<0.) ymax/=1.1; else ymax*=1.1;

    onMeanAvgFractionChange();
  }

  plotData_->axisX()->setRange(x0, x1);
  plotData_->axisY()->setRange(ymin, ymax);

//  plot_->replot();
}

void PlotWidget::onToggleY0(bool)
{
  double x0=dynamic_cast<const QtCharts::QValueAxis&>(*plotData_->axisX()).min();
  double x1=dynamic_cast<const QtCharts::QValueAxis&>(*plotData_->axisX()).max();
  QString sx0="";
  QString sx1="";
  if ( fabs(x1-x0)>0 )
  {
    sx0=QString::number(x0);
    sx1=QString::number(x1);
  }
  onChangeXRange(sx0, sx1);
}

double PlotWidget::finalRawValue() const
{
    insight::assertion(raw_crv_->count()>0, "no data available");
    return raw_crv_->at(raw_crv_->count()-1).y();
}

double PlotWidget::finalMeanValue() const
{
    insight::assertion(mean_crv_->count()>0, "no mean value computed yet");
    return mean_crv_->at(mean_crv_->count()-1).y();
}
