#include "localrun.h"
#include "analysisform.h"
#include "ui_analysisform.h"
#include "progressrelay.h"

#include <QMessageBox>




LocalRun::LocalRun(AnalysisForm *af)
  : insight::QAnalysisThread(
          af->psmodel_->getAnalysisName(),
#warning wait for visualization run to finish, if is running
          bool(af->sid_) ?
            insight::AnalysisThread::ParameterInput( af->sid_ )
          : insight::AnalysisThread::ParameterInput(
              insight::AnalysisThread::ParameterSetAndExePath{
                &af->parameters(), af->localCaseDirectory() } ),
          &af->progressDisplayer_ ),
    WorkbenchAction(af)
{
    // presumption: all signals have to be emitted from another thread!
    QObject::connect(this, &insight::QAnalysisThread::finished,
            af, &AnalysisForm::onResultReady,
            Qt::QueuedConnection);

    QObject::connect(this, &insight::QAnalysisThread::failed,
            af, &AnalysisForm::onAnalysisError,
            Qt::QueuedConnection);

    QObject::connect(this, &insight::QAnalysisThread::cancelled,
            af, &AnalysisForm::onAnalysisCancelled,
            Qt::QueuedConnection);

  af_->progressDisplayer_.reset();
  af_->ui->tabWidget->setCurrentWidget(af_->ui->runTab);
}



LocalRun::~LocalRun()
{
  interrupt();
  join();
}


std::unique_ptr<insight::ResultSet> LocalRun::moveResults()
{
    return std::move(*this);
}

void LocalRun::onCancel()
{
  interrupt();
}
