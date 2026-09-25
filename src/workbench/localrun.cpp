#include "localrun.h"
#include "analysisform.h"
#include "ui_analysisform.h"
#include "progressrelay.h"

#include <QMessageBox>




LocalRun::LocalRun(AnalysisForm *af, insight::supplementedInputDataBasePtr sid)
: WorkbenchAction(af)
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

  if (sid && sid->executionPath()!=af->localCaseDirectory())
  {
      // e.g. execution directory changed after visualization
      insight::dbg()
          << "supplemented input data was computed for execution path "
          << sid->executionPath()
          << " instead of " << af->localCaseDirectory()
          << ", recomputing" << std::endl;
      sid.reset();
  }

  QAnalysisThread::launch(
      af->psmodel_->getAnalysisName(),
      bool(sid) ?
          insight::AnalysisThread::ParameterInput( sid )
                     : insight::AnalysisThread::ParameterInput(
                           insight::AnalysisThread::ParameterSetAndExePath{
                                &af->parameters(), af->localCaseDirectory() } ),
      &af->progressDisplayer_ );
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
