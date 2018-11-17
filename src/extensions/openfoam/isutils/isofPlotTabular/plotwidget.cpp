#include "plotwidget.h"
#include "ui_plotwidget.h"

#include "qwt_plot.h"

#include "base/boost_include.h"

using namespace std;
using namespace boost;

PlotWidget::PlotWidget(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::PlotWidget)
{
  ui->setupUi(this);

  QVBoxLayout *layout=new QVBoxLayout(this);
  ui->plotcanvas->setLayout(layout);

  plot_=new QwtPlot(this);
  layout->addWidget(plot_);

  plot_->insertLegend( new QwtLegend() );
  plot_->setCanvasBackground( Qt::white );

  QwtPlotGrid *grid = new QwtPlotGrid();
  grid->attach(plot_);

  raw_crv_=new QwtPlotCurve();
  raw_crv_->setTitle("raw");
  raw_crv_->setPen(QPen(Qt::red, 2.0));
  raw_crv_->attach(plot_);

  mean_crv_=new QwtPlotCurve();
  mean_crv_->setTitle("mean");
  mean_crv_->setPen(QPen(Qt::black, 3.0));
  mean_crv_->attach(plot_);

  connect(ui->include_0_sw, &QCheckBox::toggled,
          this, &PlotWidget::onToggleY0);

}

PlotWidget::~PlotWidget()
{
  delete ui;
}

void PlotWidget::setData(const arma::mat& x, const arma::mat& y)
{
  rawdata_=arma::join_horiz(x,y);

  raw_crv_->setSamples(x.colptr(0), y.colptr(0), x.n_rows);
  double dummy=0.;
  mean_crv_->setSamples(&dummy, &dummy, 0);

  ui->final_vals->setText(QString::fromStdString(
    str(format("Final values: raw=%g / mean = (-)")%y(y.n_rows-1))
  ));

  onToggleY0(true);
}

void PlotWidget::onShow()
{
  if (mean_crv_->data()->size()==0)
  {
    ui->info->setText("Computing moving average of raw data...");
    mc_=new MeanComputer(this, rawdata_);
    connect(mc_, &MeanComputer::resultReady, this, &PlotWidget::onMeanDataReady);
    mc_->start();
  }
}

MeanComputer::MeanComputer(QObject* p, const arma::mat& rd)
  : QThread(p),
    rawdata_(rd)
{}

void MeanComputer::run()
{
  emit resultReady( insight::movingAverage(rawdata_, 0.25) );
}

void PlotWidget::onMeanDataReady(arma::mat avg)
{
  ui->final_vals->setText(QString::fromStdString(
    str(format("Final values: raw=%g / mean = %g")
         %( rawdata_.col(1)(rawdata_.n_rows-1) )
         %( avg.col(1)(avg.n_rows-1) ))
  ));

  mean_crv_->setSamples(avg.colptr(0), avg.colptr(1), avg.n_rows);
  ui->info->setText("Moving average computed.");
  plot_->replot();
}


void PlotWidget::onChangeX0(double x0)
{
  double x1=rawdata_.col(0).max();

  arma::mat visible_part=rawdata_.rows( arma::find(rawdata_.col(0)>x0) );
  double ymin=visible_part.col(1).min();
  double ymax=visible_part.col(1).max();

  if (ui->include_0_sw->isChecked())
  {
    if (ymin>0.) ymin=0.;
    if (ymax<0.) ymax=0.;
  }

  if (ymin<0.) ymin*=1.1; else ymin/=1.1;
  if (ymax<0.) ymax/=1.1; else ymax*=1.1;

  plot_->setAxisScale(QwtPlot::xBottom, x0, x1);
  plot_->setAxisScale(QwtPlot::yLeft, ymin, ymax);

  plot_->replot();
}

void PlotWidget::onToggleY0(bool)
{
  onChangeX0( plot_->axisInterval(QwtPlot::xBottom).minValue() );
}
