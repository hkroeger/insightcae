#include "plotwidget.h"
#include "ui_plotwidget.h"

#include "qwt_plot.h"

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
}

PlotWidget::~PlotWidget()
{
  delete ui;
}

void PlotWidget::setData(const arma::mat& x, const arma::mat& y)
{
  raw_crv_->setSamples(x.colptr(0), y.colptr(0), x.n_rows);

  arma::mat avg=insight::movingAverage(arma::join_horiz(x, y));

  mean_crv_->setSamples(avg.colptr(0), avg.colptr(1), avg.n_rows);
}
