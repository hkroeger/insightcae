#include "workbenchaction.h"
#include "analysisform.h"
#include "ui_analysisform.h"

#include "base/qt5_helper.h"


void WorkbenchAction::exceptionEmitter()
{
  try
  {
    throw;
  }
  catch (boost::thread_interrupted i)
  {
    Q_EMIT killed();
  }
  catch (const std::exception& e)
  {
    if ( const auto* ie = dynamic_cast<const insight::Exception*>(&e) )
    {
      Q_EMIT failed( insight::Exception(*ie) );
    }
    else
    {
      Q_EMIT failed( insight::Exception(e.what()) );
    }
  }
}


WorkbenchAction::WorkbenchAction(AnalysisForm *af)
  : af_(af)
{
  af_->ui->gbExecution->setEnabled(false);
  af_->ui->btnRun->setEnabled(false);
  af_->ui->btnKill->setEnabled(true);

  // presumption: all signals have to be emitted from another thread!
  connect(this, &WorkbenchAction::failed,
          af_, &AnalysisForm::onAnalysisErrorOccurred,
          Qt::QueuedConnection);

  connect(this, &WorkbenchAction::warning,
          af_, &AnalysisForm::onAnalysisWarningOccurred,
          Qt::QueuedConnection);

  connect(this, &WorkbenchAction::finished,
          af_, &AnalysisForm::onResultReady,
          Qt::QueuedConnection);

  connect(this, &WorkbenchAction::killed,
          af_, &AnalysisForm::onAnalysisKilled,
          Qt::QueuedConnection);

  connect(this, &WorkbenchAction::analysisProgressUpdate,
          af_->progdisp_, &GraphProgressDisplayer::update,
          Qt::QueuedConnection);

  connect(this, &WorkbenchAction::analysisProgressUpdate,
          af_->log_, &LogViewerWidget::appendLogMessage,
          Qt::QueuedConnection);
}


WorkbenchAction::~WorkbenchAction()
{
  af_->ui->gbExecution->setEnabled(true);
  af_->ui->btnRun->setEnabled(true);
  af_->ui->btnKill->setEnabled(false);
}
