#include "localrun.h"
#include "analysisform.h"
#include "ui_analysisform.h"
#include "progressrelay.h"

#include <QMessageBox>




LocalRun::LocalRun(AnalysisForm *af)
  : WorkbenchAction(af),
    workerThread_(
          af_->analysisName_,
#warning wait for visualization run to finish, if is running
          bool(af_->sid_) ?
            insight::AnalysisThread::ParameterInput( af_->sid_ )
          : insight::AnalysisThread::ParameterInput(
              insight::AnalysisThread::ParameterSetAndExePath{
                &af_->parameters(), af_->localCaseDirectory() } ),
          &af->progressDisplayer_ )
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
