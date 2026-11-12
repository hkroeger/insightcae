#include "convergencewindow.h"
#include "casewidget.h"
#include "statustabwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDoubleValidator>
#include <QComboBox>
#include <QSignalBlocker>

#include <cfloat>
#include <cmath>
#include <limits>
#include <iostream>


EvaluationWorker::EvaluationWorker(
    QObject* parent,
    const std::vector<CaseData>& cases,
    const std::vector<Job>& jobs,
    double fraction )
  : QThread(parent),
    cases_(cases),
    jobs_(jobs),
    fraction_(fraction)
{}


void EvaluationWorker::run()
{
  for (const auto& j: jobs_)
  {
    if (isInterruptionRequested()) return;
    // note: only the (constant) time series data of the cases is accessed here
    Q_EMIT caseEvaluated(
        j.generation, j.caseIndex,
        evaluate(cases_[j.caseIndex], fraction_, j.t0, j.t1) );
  }
}




ConvergenceWindow::ConvergenceWindow(
    const boost::filesystem::path& dataDir,
    std::vector<CaseData>& cases,
    double fraction, const Criterion& crit,
    double t0, double t1 )
  : QMainWindow(),
    dataDir_(dataDir),
    cases_(cases),
    requestedGeneration_(cases.size(), 0),
    resultGeneration_(cases.size(), -1)
{
  qRegisterMetaType<std::vector<ColumnResult> >();

  setWindowTitle("ConvergenceByVariance: "+QString::fromStdString(dataDir.string()));

  double tmin=DBL_MAX, tmax=-DBL_MAX;
  for (const auto& cd: cases_)
    for (const auto& s: cd.series)
    {
      tmin=std::min(tmin, s.tx.col(0).min());
      tmax=std::max(tmax, s.tx.col(0).max());
    }

  auto *cw = new QWidget(this);
  auto *layout = new QVBoxLayout(cw);
  setCentralWidget(cw);

  // global evaluation parameters
  auto *ctrl = new QHBoxLayout;
  layout->addLayout(ctrl);

  ctrl->addWidget(new QLabel("Global time window:  start"));
  startTime_ = new QLineEdit;
  startTime_->setValidator(new QDoubleValidator(startTime_));
  startTime_->setPlaceholderText(QString::number(tmin)+" (per series)");
  startTime_->setToolTip("used for all cases without case specific time window.\n"
                         "empty: first time of each series");
  ctrl->addWidget(startTime_);

  ctrl->addWidget(new QLabel("end"));
  endTime_ = new QLineEdit;
  endTime_->setValidator(new QDoubleValidator(endTime_));
  endTime_->setPlaceholderText(QString::number(tmax)+" (per series)");
  endTime_->setToolTip("used for all cases without case specific time window.\n"
                       "empty: last time of each series");
  ctrl->addWidget(endTime_);

  auto *resetBtn = new QPushButton("Reset");
  ctrl->addWidget(resetBtn);

  ctrl->addWidget(new QLabel("  Window fraction:"));
  fraction_ = new QDoubleSpinBox;
  fraction_->setDecimals(3);
  fraction_->setRange(0.001, 1.0);
  fraction_->setSingleStep(0.05);
  fraction_->setValue(fraction);
  fraction_->setToolTip("trailing window size as fraction of the number of evaluated samples");
  ctrl->addWidget(fraction_);

  ctrl->addWidget(new QLabel("  CoV threshold:"));
  threshold_ = new QLineEdit(QString::number(crit.threshold));
  auto *tv = new QDoubleValidator(threshold_);
  tv->setBottom(0.);
  threshold_->setValidator(tv);
  threshold_->setMaximumWidth(100);
  threshold_->setToolTip("a series is converged, if its final CoV is below this value");
  ctrl->addWidget(threshold_);

  ctrl->addWidget(new QLabel("  Min. window:"));
  // initial values per unit: given one from criterion, the other a default
  lastMinWindow_[int(WindowUnit::Samples)] = 50;
  lastMinWindow_[int(WindowUnit::Time)] = (tmax>tmin) ? 0.1*(tmax-tmin) : 1.;
  lastMinWindow_[int(crit.unit)] = crit.minWindow;
  minWindow_ = new QDoubleSpinBox;
  minWindow_->setToolTip("a series is only considered converged, if its trailing window\n"
                         "contains at least this number of samples or covers at least this time span\n"
                         "(suppresses start-up effects)");
  ctrl->addWidget(minWindow_);
  windowUnit_ = new QComboBox;
  windowUnit_->addItem("samples");   // index == WindowUnit::Samples
  windowUnit_->addItem("time span"); // index == WindowUnit::Time
  windowUnit_->setToolTip("unit of the min. window size");
  windowUnit_->setCurrentIndex(int(crit.unit));
  ctrl->addWidget(windowUnit_);
  setupMinWindowSpinBox(crit.unit);
  ctrl->addStretch();

  // one tab per case
  tabs_ = new StatusTabWidget;
  layout->addWidget(tabs_);
  for (size_t i=0; i<cases_.size(); ++i)
  {
    auto *w = new CaseWidget(cases_[i], tabs_);
    w->setCriterion(crit);
    connect(w, &CaseWidget::timeWindowChanged,
            this, [this,i]() { requestEvaluation(int(i)); });
    connect(w, &CaseWidget::selectionChanged,
            this, [this]() { updateStatus(); storeSelection(); });
    tabs_->addTab(w, QString::fromStdString(cases_[i].name));
    caseWidgets_.push_back(w);
  }

  summary_ = new QLabel;
  layout->addWidget(summary_);

  if (!std::isnan(t0)) startTime_->setText(QString::number(t0));
  if (!std::isnan(t1)) endTime_->setText(QString::number(t1));

  updateTimer_.setSingleShot(true);
  updateTimer_.setInterval(250);
  connect(&updateTimer_, &QTimer::timeout, this, &ConvergenceWindow::startEvaluation);

  connect(startTime_, &QLineEdit::editingFinished, this, [this]() { requestEvaluation(); });
  connect(endTime_, &QLineEdit::editingFinished, this, [this]() { requestEvaluation(); });
  connect(fraction_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, [this]() { requestEvaluation(); });
  connect(threshold_, &QLineEdit::editingFinished, this, &ConvergenceWindow::onCriterionChanged);
  connect(minWindow_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ConvergenceWindow::onCriterionChanged);
  connect(windowUnit_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ConvergenceWindow::onWindowUnitChanged);
  connect(resetBtn, &QPushButton::clicked, this, &ConvergenceWindow::resetGlobalTimeWindow);

  resize(1400, 850);
  requestEvaluation();
  updateTimer_.stop();
  startEvaluation();
}


ConvergenceWindow::~ConvergenceWindow()
{
  if (worker_)
  {
    worker_->requestInterruption();
    worker_->wait();
  }
}


Criterion ConvergenceWindow::criterion() const
{
  Criterion c;
  bool ok;
  double v=threshold_->text().toDouble(&ok);
  if (ok && v>0.) c.threshold=v;
  c.unit=WindowUnit(windowUnit_->currentIndex());
  c.minWindow=minWindow_->value();
  return c;
}


void ConvergenceWindow::setupMinWindowSpinBox(WindowUnit u)
{
  QSignalBlocker b(minWindow_);
  if (u==WindowUnit::Samples)
  {
    minWindow_->setDecimals(0);
    minWindow_->setRange(2, 1e6); // window of 1 sample: no variance
    minWindow_->setSingleStep(1);
    minWindow_->setSuffix(" samples");
  }
  else
  {
    double v=lastMinWindow_[int(u)];
    minWindow_->setDecimals(6);
    minWindow_->setRange(1e-6, 1e12);
    minWindow_->setSingleStep(v>0. ? std::pow(10., std::floor(std::log10(v))) : 1.);
    minWindow_->setSuffix("");
  }
  minWindow_->setValue(lastMinWindow_[int(u)]);
}


void ConvergenceWindow::onWindowUnitChanged()
{
  // remember value of previous unit
  auto newUnit = WindowUnit(windowUnit_->currentIndex());
  auto oldUnit = newUnit==WindowUnit::Samples ? WindowUnit::Time : WindowUnit::Samples;
  lastMinWindow_[int(oldUnit)] = minWindow_->value();
  setupMinWindowSpinBox(newUnit);
  onCriterionChanged();
}


void ConvergenceWindow::effectiveTimeWindow(int caseIndex, double& t0, double& t1) const
{
  // case specific setting, then global setting, then range of series (NaN)
  caseWidgets_[caseIndex]->timeWindow(t0, t1);
  bool ok;
  double v;
  if (std::isnan(t0)) { v=startTime_->text().toDouble(&ok); if (ok) t0=v; }
  if (std::isnan(t1)) { v=endTime_->text().toDouble(&ok); if (ok) t1=v; }
}


void ConvergenceWindow::requestEvaluation(int caseIndex)
{
  for (size_t i=0; i<cases_.size(); ++i)
    if (caseIndex<0 || int(i)==caseIndex)
      ++requestedGeneration_[i];
  updateStatus();
  updateTimer_.start();
}


void ConvergenceWindow::startEvaluation()
{
  if (worker_)
  {
    // let running evaluation stop first, restart when finished
    worker_->requestInterruption();
    restartPending_=true;
    return;
  }

  // evaluate all cases, which have no up-to-date results
  std::vector<EvaluationWorker::Job> jobs;
  for (size_t i=0; i<cases_.size(); ++i)
  {
    if (resultGeneration_[i]!=requestedGeneration_[i])
    {
      EvaluationWorker::Job j;
      j.caseIndex=int(i);
      j.generation=requestedGeneration_[i];
      effectiveTimeWindow(int(i), j.t0, j.t1);
      jobs.push_back(j);
    }
  }
  if (jobs.empty()) return;

  worker_ = new EvaluationWorker(this, cases_, jobs, fraction_->value());
  connect(worker_, &EvaluationWorker::caseEvaluated,
          this, &ConvergenceWindow::onCaseEvaluated, Qt::QueuedConnection);
  connect(worker_, &QThread::finished, this, [this]()
  {
    worker_->deleteLater();
    worker_=nullptr;
    if (restartPending_)
    {
      restartPending_=false;
      startEvaluation();
    }
  });
  worker_->start();
}


void ConvergenceWindow::onCaseEvaluated(int generation, int caseIndex, std::vector<ColumnResult> results)
{
  if (generation!=requestedGeneration_[caseIndex]) return; // outdated

  caseWidgets_[caseIndex]->setResults(results);
  resultGeneration_[caseIndex]=generation;
  updateStatus();
}


void ConvergenceWindow::onCriterionChanged()
{
  auto c=criterion();
  for (auto* w: caseWidgets_)
    w->setCriterion(c);
  updateStatus();
}


void ConvergenceWindow::updateStatus()
{
  int nConv=0, nEarlier=0, nNotConv=0, nDone=0, nNoSel=0;
  for (size_t i=0; i<caseWidgets_.size(); ++i)
  {
    auto *w=caseWidgets_[i];
    QColor c; // default: pending or no series selected
    if (resultGeneration_[i]==requestedGeneration_[i] && w->hasResults())
    {
      ++nDone;
      switch (w->status())
      {
        case CaseStatus::Converged: ++nConv; c=CaseWidget::convergedColor; break;
        case CaseStatus::ConvergedEarlier: ++nEarlier; c=CaseWidget::convergedEarlierColor; break;
        case CaseStatus::NotConverged: ++nNotConv; c=CaseWidget::notConvergedColor; break;
        case CaseStatus::NoSelection: ++nNoSel; break;
      }
    }
    tabs_->setTabStatusColor(int(i), c);
  }

  auto crit=criterion();
  QString s = QString("Of %1 cases: %2 converged, %3 converged earlier (diverged again), %4 not converged"
                      "   (CoV threshold %5, min. window %6)")
                  .arg(cases_.size()).arg(nConv).arg(nEarlier).arg(nNotConv)
                  .arg(crit.threshold).arg(QString::fromStdString(minWindowText(crit)));
  if (nNoSel>0)
    s += QString(", %1 without selected series").arg(nNoSel);
  if (nDone<int(cases_.size()))
    s += QString("  —  evaluating... (%1 of %2 cases done)").arg(nDone).arg(cases_.size());
  summary_->setText(s);
}


void ConvergenceWindow::storeSelection()
{
  try
  {
    saveSelection(dataDir_, cases_);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    summary_->setText(summary_->text()+"  —  could not store selection: "+e.what());
  }
}


void ConvergenceWindow::resetGlobalTimeWindow()
{
  startTime_->clear();
  endTime_->clear();
  requestEvaluation();
}
