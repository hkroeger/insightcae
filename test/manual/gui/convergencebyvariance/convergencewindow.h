#ifndef CONVERGENCEWINDOW_H
#define CONVERGENCEWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QThread>
#include <QTimer>

#include <boost/filesystem/path.hpp>

#include "casereader.h"
#include "evaluate.h"

class CaseWidget;
class StatusTabWidget;

Q_DECLARE_METATYPE(std::vector<ColumnResult>)


/**
 * evaluates a list of cases in a background thread
 */
class EvaluationWorker : public QThread
{
  Q_OBJECT

public:
  struct Job
  {
    int caseIndex, generation;
    double t0, t1; // NaN: time range of each series
  };

private:
  const std::vector<CaseData>& cases_;
  std::vector<Job> jobs_;
  double fraction_;

public:
  EvaluationWorker(
      QObject* parent,
      const std::vector<CaseData>& cases,
      const std::vector<Job>& jobs,
      double fraction );

  void run() override;

Q_SIGNALS:
  void caseEvaluated(int generation, int caseIndex, std::vector<ColumnResult> results);
};


class ConvergenceWindow : public QMainWindow
{
  Q_OBJECT

  boost::filesystem::path dataDir_;
  std::vector<CaseData>& cases_;

  QLineEdit *startTime_, *endTime_, *threshold_;
  QDoubleSpinBox *fraction_;
  QComboBox *windowUnit_;
  QDoubleSpinBox *minWindow_;
  double lastMinWindow_[2]; // last value per WindowUnit (index: Samples, Time)
  StatusTabWidget *tabs_;
  QLabel *summary_;
  std::vector<CaseWidget*> caseWidgets_;

  QTimer updateTimer_;
  EvaluationWorker *worker_ = nullptr;
  bool restartPending_ = false;
  std::vector<int> requestedGeneration_; // per case: latest requested evaluation
  std::vector<int> resultGeneration_;    // per case: evaluation of displayed results

  Criterion criterion() const;
  void effectiveTimeWindow(int caseIndex, double& t0, double& t1) const;
  void requestEvaluation(int caseIndex = -1); // -1: all cases
  void startEvaluation();
  void updateStatus();
  void storeSelection();
  void setupMinWindowSpinBox(WindowUnit u);

public:
  /**
   * t0, t1: initial global time window, NaN = time range of each series
   */
  ConvergenceWindow(
      const boost::filesystem::path& dataDir,
      std::vector<CaseData>& cases,
      double fraction, const Criterion& crit,
      double t0, double t1 );
  ~ConvergenceWindow();

public Q_SLOTS:
  void onCriterionChanged();
  void onWindowUnitChanged();
  void onCaseEvaluated(int generation, int caseIndex, std::vector<ColumnResult> results);
  void resetGlobalTimeWindow();
};

#endif
