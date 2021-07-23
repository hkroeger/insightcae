#include "localrun.h"
#include "analysisform.h"
#include "ui_analysisform.h"
#include "progressrelay.h"

#include <QMessageBox>




LocalRun::LocalRun(AnalysisForm *af)
  : WorkbenchAction(af),
    analysis_(
          insight::Analysis::lookup(
            af_->analysisName_,
            af_->parameters(),
            af_->localCaseDirectory()
            )
          ),
    workerThread_(analysis_, &af->progressDisplayer_)
{
  connectAnalysisThread(&workerThread_);
  af_->progressDisplayer_.reset();
  af_->ui->tabWidget->setCurrentWidget(af_->ui->runTab);
}



LocalRun::~LocalRun()
{
  workerThread_.interrupt();
  workerThread_.join();

  af_->progressDisplayer_.reset();
}


void LocalRun::onCancel()
{
  workerThread_.interrupt();
}
