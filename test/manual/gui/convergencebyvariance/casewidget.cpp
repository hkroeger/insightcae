#include "casewidget.h"
#include "convergenceplotwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <QDoubleValidator>
#include <QSignalBlocker>

#include <cmath>
#include <limits>

const QColor CaseWidget::convergedColor("#7bc96f");
const QColor CaseWidget::convergedEarlierColor("#ffd43b");
const QColor CaseWidget::notConvergedColor("#ef5350");


CaseWidget::CaseWidget(CaseData& cd, QWidget* parent)
  : QWidget(parent),
    case_(cd)
{
  auto *layout = new QVBoxLayout(this);

  // case specific time window
  auto *tw = new QHBoxLayout;
  layout->addLayout(tw);
  tw->addWidget(new QLabel("Time window of this case:  start"));
  startTime_ = new QLineEdit;
  startTime_->setValidator(new QDoubleValidator(startTime_));
  startTime_->setPlaceholderText("global setting");
  tw->addWidget(startTime_);
  tw->addWidget(new QLabel("end"));
  endTime_ = new QLineEdit;
  endTime_->setValidator(new QDoubleValidator(endTime_));
  endTime_->setPlaceholderText("global setting");
  tw->addWidget(endTime_);
  auto *resetBtn = new QPushButton("Use global window");
  tw->addWidget(resetBtn);
  tw->addStretch();

  connect(startTime_, &QLineEdit::editingFinished, this, &CaseWidget::timeWindowChanged);
  connect(endTime_, &QLineEdit::editingFinished, this, &CaseWidget::timeWindowChanged);
  connect(resetBtn, &QPushButton::clicked, this, &CaseWidget::resetTimeWindow);

  if (!case_.warnings.empty())
  {
    QStringList w;
    for (const auto& m: case_.warnings)
      w << QString::fromStdString(m);
    auto *wl = new QLabel("<b>Skipped files:</b> "+w.join("; ").toHtmlEscaped());
    wl->setWordWrap(true);
    wl->setTextFormat(Qt::RichText);
    layout->addWidget(wl);
  }

  auto *splitter = new QSplitter(Qt::Horizontal);
  layout->addWidget(splitter);

  list_ = new QListWidget;
  list_->setToolTip("green: converged at the end of the time window\n"
                    "yellow: converged earlier, but not at the end\n"
                    "red: never converged\n"
                    "unchecked series are ignored in the convergence status of the case");
  for (const auto& s: case_.series)
  {
    auto *item = new QListWidgetItem(QString::fromStdString(s.label()));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(s.selected ? Qt::Checked : Qt::Unchecked);
    list_->addItem(item);
  }
  splitter->addWidget(list_);

  plot_ = new ConvergencePlotWidget;
  splitter->addWidget(plot_);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  splitter->setSizes({300, 900});

  connect(plot_, &ConvergencePlotWidget::timeWindowSelected,
          this, &CaseWidget::setTimeWindow);
  connect(plot_, &ConvergencePlotWidget::timeWindowReset,
          this, &CaseWidget::resetTimeWindow);
  connect(list_, &QListWidget::currentRowChanged,
          this, &CaseWidget::onCurrentRowChanged);
  connect(list_, &QListWidget::itemChanged,
          this, &CaseWidget::onItemChanged);

  if (list_->count()>0)
    list_->setCurrentRow(0);
}


void CaseWidget::onCurrentRowChanged(int row)
{
  if (row<0 || row>=int(case_.series.size())) return;
  plot_->setSeries(case_.series[row].tx, QString::fromStdString(case_.series[row].label()));
  if (row<int(results_.size()))
    plot_->showResult(results_[row], crit_);
}


void CaseWidget::onItemChanged(QListWidgetItem* item)
{
  int row=list_->row(item);
  if (row<0 || row>=int(case_.series.size())) return;
  bool sel = item->checkState()==Qt::Checked;
  if (sel!=case_.series[row].selected)
  {
    case_.series[row].selected=sel;
    updateItems();
    Q_EMIT selectionChanged();
  }
}


void CaseWidget::setResults(const std::vector<ColumnResult>& results)
{
  results_=results;
  setCriterion(crit_);
}


void CaseWidget::setCriterion(const Criterion& c)
{
  crit_=c;
  updateItems();
  int row=list_->currentRow();
  if (row>=0 && row<int(results_.size()))
    plot_->showResult(results_[row], crit_);
}


void CaseWidget::updateItems()
{
  QSignalBlocker block(list_); // text/color changes would trigger itemChanged
  for (size_t i=0; i<results_.size() && int(i)<list_->count(); ++i)
  {
    const auto& r = results_[i];
    const auto& s = case_.series[i];
    auto *item = list_->item(i);
    QString lbl = QString::fromStdString(s.label());
    if (r.nSamples>0)
      item->setText(lbl+QString("\nCoV = %1").arg(r.finalCoV, 0, 'g', 3));
    else
      item->setText(lbl+"\n(no samples in window)");

    if (s.selected)
    {
      switch (seriesStatus(r, crit_))
      {
        case SeriesStatus::Converged: item->setBackground(convergedColor); break;
        case SeriesStatus::ConvergedEarlier: item->setBackground(convergedEarlierColor); break;
        case SeriesStatus::NotConverged: item->setBackground(notConvergedColor); break;
      }
    }
    else
      item->setBackground(QBrush()); // ignored: normal background
  }

  // depends on results, threshold and selection
  plot_->setConvergedTime(
      results_.empty() ? std::numeric_limits<double>::quiet_NaN()
                       : firstConvergedTime(case_, results_, crit_) );
}


void CaseWidget::timeWindow(double& t0, double& t1) const
{
  t0=t1=std::numeric_limits<double>::quiet_NaN();
  bool ok;
  double v;
  v=startTime_->text().toDouble(&ok); if (ok) t0=v;
  v=endTime_->text().toDouble(&ok); if (ok) t1=v;
}


void CaseWidget::setTimeWindow(double t0, double t1)
{
  startTime_->setText(QString::number(t0));
  endTime_->setText(QString::number(t1));
  Q_EMIT timeWindowChanged();
}


void CaseWidget::resetTimeWindow()
{
  startTime_->clear();
  endTime_->clear();
  Q_EMIT timeWindowChanged();
}


CaseStatus CaseWidget::status() const
{
  return caseStatus(case_, results_, crit_);
}


bool CaseWidget::hasResults() const
{
  return !results_.empty() || case_.series.empty();
}
