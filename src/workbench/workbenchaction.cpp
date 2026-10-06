#include "workbenchaction.h"
#include "analysisform.h"
#include "ui_analysisform.h"

#include "base/qt5_helper.h"
#include "qanalysisthread.h"

using namespace insight;



WorkbenchAction::WorkbenchAction(AnalysisForm *af)
  : af_(af)
{
  af_->ui->btnRun->setEnabled(false);
  if (af_->act_run_) af_->act_run_->setEnabled(false);
  af_->ui->btnKill->setEnabled(true);

  // // presumption: all signals have to be emitted from another thread!
  // connect(this, &WorkbenchAction::finished,
  //         af_, &AnalysisForm::onResultReady,
  //         Qt::QueuedConnection);

  // connect(this, &WorkbenchAction::failed,
  //         af_, &AnalysisForm::onAnalysisError,
  //         Qt::QueuedConnection);

  // connect(this, &WorkbenchAction::cancelled,
  //         af_, &AnalysisForm::onAnalysisCancelled,
  //         Qt::QueuedConnection);

  // connect(this, &WorkbenchAction::statusMessage,
  //         [this](const QString& msg) { af_->statusMessage(msg); }
  // );

}


WorkbenchAction::~WorkbenchAction()
{
  af_->updateRunAvailability(); // run is not possible, if the input data has errors
  af_->ui->btnKill->setEnabled(false);
}
