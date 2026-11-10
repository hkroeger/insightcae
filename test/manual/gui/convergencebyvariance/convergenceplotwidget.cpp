#include "convergenceplotwidget.h"

#include <cmath>
#include <limits>

#include <QVBoxLayout>
#include <QMouseEvent>
#include <QSplitter>
#include <QtCharts/QLegendMarker>

#include "boost/format.hpp"

using namespace QtCharts;


namespace {

QVector<QPointF> toPoints(const arma::mat& xy, arma::uword ycol=1, double ymin=-DBL_MAX)
{
  QVector<QPointF> pts;
  pts.reserve(xy.n_rows);
  for (arma::uword i=0; i<xy.n_rows; ++i)
    pts.append(QPointF(xy(i,0), std::max(ymin, xy(i,ycol))));
  return pts;
}

void axisRange(QValueAxis* ax, double a, double b)
{
  if (b-a < 1e-12*std::max(1.0, std::max(fabs(a), fabs(b))))
  {
    double d=std::max(1e-12, 0.05*fabs(a));
    a-=d; b+=d;
  }
  ax->setRange(a, b);
}

}



TimeWindowChartView::TimeWindowChartView(QChart* chart, QWidget* parent)
  : QChartView(chart, parent),
    rubberBand_(new QRubberBand(QRubberBand::Rectangle, this))
{
  setRenderHint(QPainter::Antialiasing);
  setToolTip("Drag with left mouse button to select the evaluation time window.\n"
             "Double click to reset to full time range.");
}

double TimeWindowChartView::xValue(const QPoint& p) const
{
  return chart()->mapToValue(chart()->mapFromScene(mapToScene(p))).x();
}

void TimeWindowChartView::mousePressEvent(QMouseEvent* e)
{
  if (e->button()==Qt::LeftButton)
  {
    origin_=e->pos();
    dragging_=true;
    QRect pa=chart()->plotArea().toRect();
    rubberBand_->setGeometry(QRect(origin_.x(), pa.top(), 0, pa.height()));
    rubberBand_->show();
    e->accept();
  }
  else
    QChartView::mousePressEvent(e);
}

void TimeWindowChartView::mouseMoveEvent(QMouseEvent* e)
{
  if (dragging_)
  {
    QRect pa=chart()->plotArea().toRect();
    rubberBand_->setGeometry(
        QRect(QPoint(origin_.x(), pa.top()), QPoint(e->pos().x(), pa.bottom()))
            .normalized() );
    e->accept();
  }
  else
    QChartView::mouseMoveEvent(e);
}

void TimeWindowChartView::mouseReleaseEvent(QMouseEvent* e)
{
  if (dragging_ && e->button()==Qt::LeftButton)
  {
    dragging_=false;
    rubberBand_->hide();
    if (std::abs(e->pos().x()-origin_.x())>3)
    {
      double t0=xValue(origin_), t1=xValue(e->pos());
      if (t1<t0) std::swap(t0, t1);
      Q_EMIT timeWindowSelected(t0, t1);
    }
    e->accept();
  }
  else
    QChartView::mouseReleaseEvent(e);
}

void TimeWindowChartView::mouseDoubleClickEvent(QMouseEvent* e)
{
  if (e->button()==Qt::LeftButton)
  {
    Q_EMIT timeWindowReset();
    e->accept();
  }
  else
    QChartView::mouseDoubleClickEvent(e);
}




ConvergencePlotWidget::ConvergencePlotWidget(QWidget* parent)
  : QWidget(parent)
{
  auto *layout = new QVBoxLayout(this);
  auto *splitter = new QSplitter(Qt::Vertical, this);
  layout->addWidget(splitter);

  // signal chart
  signalChart_ = new QChart;
  signalChart_->setTitle("Signal (drag to select evaluation window)");
  sx_ = new QValueAxis; sx_->setTitleText("t");
  sy_ = new QValueAxis; sy_->setTitleText("value");
  signalChart_->addAxis(sx_, Qt::AlignBottom);
  signalChart_->addAxis(sy_, Qt::AlignLeft);

  auto addSeries = [](QChart* c, QAbstractAxis* ax, QAbstractAxis* ay,
                      const QString& name, const QPen& pen)
  {
    auto *s = new QLineSeries;
    s->setName(name);
    s->setPen(pen);
    c->addSeries(s);
    s->attachAxis(ax);
    s->attachAxis(ay);
    return s;
  };

  raw_ = addSeries(signalChart_, sx_, sy_, "raw", QPen(Qt::gray, 1.5));
  selected_ = addSeries(signalChart_, sx_, sy_, "evaluated", QPen(Qt::red, 2.0));
  startMarker_ = addSeries(signalChart_, sx_, sy_, "window start", QPen(Qt::blue, 1.5, Qt::DashLine));
  endMarker_ = addSeries(signalChart_, sx_, sy_, "window end", QPen(Qt::blue, 1.5, Qt::DashLine));

  convergedMarker_ = addSeries(signalChart_, sx_, sy_, "all converged", QPen(QColor("#2b8a3e"), 2.5));
  convergedTime_ = std::numeric_limits<double>::quiet_NaN();
  updateConvergedMarker();

  for (auto* m: signalChart_->legend()->markers(startMarker_)) m->setVisible(false);
  for (auto* m: signalChart_->legend()->markers(endMarker_)) m->setVisible(false);
  signalChart_->legend()->setAlignment(Qt::AlignRight);

  auto *sv = new TimeWindowChartView(signalChart_, splitter);
  connect(sv, &TimeWindowChartView::timeWindowSelected,
          this, &ConvergencePlotWidget::timeWindowSelected);
  connect(sv, &TimeWindowChartView::timeWindowReset,
          this, &ConvergencePlotWidget::timeWindowReset);
  splitter->addWidget(sv);

  // convergence measure chart
  covChart_ = new QChart;
  covChart_->setTitle("Convergence measure (CoV in trailing window)");
  cx_ = new QValueAxis; cx_->setTitleText("t");
  cy_ = new QLogValueAxis; cy_->setTitleText("CoV"); cy_->setLabelFormat("%g");
  covChart_->addAxis(cx_, Qt::AlignBottom);
  covChart_->addAxis(cy_, Qt::AlignLeft);
  cov_ = addSeries(covChart_, cx_, cy_, "CoV", QPen(Qt::black, 2.0));
  threshold_ = addSeries(covChart_, cx_, cy_, "threshold", QPen(QColor("#d9480f"), 1.5, Qt::DashLine));
  covChart_->legend()->setAlignment(Qt::AlignRight);

  auto *cv = new QChartView(covChart_, splitter);
  cv->setRenderHint(QPainter::Antialiasing);
  splitter->addWidget(cv);

  splitter->setStretchFactor(0, 1);
  splitter->setStretchFactor(1, 1);
  sv->setMinimumHeight(250);
  cv->setMinimumHeight(250);

  info_ = new QLabel(this);
  info_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(info_);
}


void ConvergencePlotWidget::setSeries(const arma::mat& tx, const QString& title)
{
  tx_=tx;
  signalChart_->setTitle(title+" (drag to select evaluation window)");
  raw_->replace(toPoints(tx_));
  selected_->clear();
  startMarker_->clear();
  endMarker_->clear();
  cov_->clear();
  threshold_->clear();
  if (tx_.n_rows>0)
  {
    axisRange(sx_, tx_.col(0).min(), tx_.col(0).max());
    axisRange(sy_, tx_.col(1).min(), tx_.col(1).max());
    axisRange(cx_, tx_.col(0).min(), tx_.col(0).max());
  }
  info_->setText("evaluating...");
  updateConvergedMarker();
}


void ConvergencePlotWidget::showResult(const ColumnResult& r, const Criterion& crit)
{
  const double threshold = crit.threshold;
  const double covMin = 1e-16;
  if (tx_.n_rows==0) return;

  // highlight evaluated part of signal
  arma::uvec rows = arma::find( (tx_.col(0) >= r.t0) % (tx_.col(0) <= r.t1) );
  selected_->replace(toPoints(tx_.rows(rows)));

  // scale y axis to evaluated part (initial transients would hide the details)
  arma::vec ys = rows.n_elem>0 ? arma::vec(tx_.submat(rows, arma::uvec{1})) : arma::vec(tx_.col(1));
  double margin = 0.05*(ys.max()-ys.min());
  axisRange(sy_, ys.min()-margin, ys.max()+margin);

  double y0=sy_->min(), y1=sy_->max();
  startMarker_->replace(QVector<QPointF>{ QPointF(r.t0, y0), QPointF(r.t0, y1) });
  endMarker_->replace(QVector<QPointF>{ QPointF(r.t1, y0), QPointF(r.t1, y1) });
  updateConvergedMarker();

  // convergence history
  if (r.nSamples>0)
  {
    // log axis: base range on positive values only,
    // zeros (e.g. first sample, constant signal) are drawn at the lower bound
    arma::vec c = r.t_cov.col(1);
    arma::vec pos = c(arma::find(c>covMin));
    double cmin=threshold/2., cmax=threshold*2.;
    if (pos.n_elem>0)
    {
      cmin=std::min(cmin, pos.min()/2.);
      cmax=std::max(cmax, pos.max()*2.);
    }
    cov_->replace(toPoints(r.t_cov, 1, cmin));
    cy_->setRange(cmin, cmax);
  }
  else
  {
    cov_->clear();
    cy_->setRange(threshold/2., threshold*2.);
  }
  threshold_->replace(QVector<QPointF>{
      QPointF(cx_->min(), threshold), QPointF(cx_->max(), threshold) });
  bool ok = converged(r, crit);
  bool tooSmall = windowTooSmall(r, crit);

  if (r.nSamples>0)
    info_->setText(QString::fromStdString(
        boost::str(boost::format(
            "final CoV = %g  %s threshold %g  =>  %s    (n = %d samples, trailing window %d samples / span %g (min. %s), time window = [%g, %g])")
                   % r.finalCoV % (r.finalCoV<threshold ? "<" : ">=") % threshold
                   % (ok ? "converged" :
                      tooSmall ? "NOT converged (trailing window too small)" :
                      seriesStatus(r, crit)==SeriesStatus::ConvergedEarlier ? "NOT converged (was converged earlier)" :
                                                                               "NOT converged")
                   % r.nSamples % r.window(r.nSamples-1) % r.windowSpan(r.nSamples-1)
                   % minWindowText(crit) % r.t0 % r.t1 ) ));
  else
    info_->setText(QString::fromStdString(
        boost::str(boost::format(
            "no samples in time window [%g, %g]") % r.t0 % r.t1 ) ));
}



void ConvergencePlotWidget::setConvergedTime(double t)
{
  convergedTime_=t;
  updateConvergedMarker();
}


void ConvergencePlotWidget::updateConvergedMarker()
{
  bool show = !std::isnan(convergedTime_) && tx_.n_rows>0;
  if (show)
  {
    double y0=sy_->min(), y1=sy_->max();
    convergedMarker_->replace(QVector<QPointF>{
        QPointF(convergedTime_, y0), QPointF(convergedTime_, y1) });
    convergedMarker_->setName(QString("all selected converged\nfirst at t=%1").arg(convergedTime_, 0, 'g', 6));
  }
  else
    convergedMarker_->clear();

  for (auto* m: signalChart_->legend()->markers(convergedMarker_))
    m->setVisible(show);
}
