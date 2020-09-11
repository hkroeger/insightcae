#include "workbenchaction.h"
#include "analysisform.h"
#include "ui_analysisform.h"

#include "base/qt5_helper.h"
#include "qanalysisthread.h"

using namespace insight;


void WorkbenchAction::connectAnalysisThread(insight::QAnalysisThread *t)
{
  connect(t, &QAnalysisThread::finished,
          this, &WorkbenchAction::finished,
          Qt::QueuedConnection);
  connect(t, &QAnalysisThread::cancelled,
          this, &WorkbenchAction::cancelled,
          Qt::QueuedConnection);
  connect(t, &QAnalysisThread::failed,
          this, &WorkbenchAction::failed,
          Qt::QueuedConnection);
}

WorkbenchAction::WorkbenchAction(AnalysisForm *af)
  : af_(af)
{
  af_->ui->btnRun->setEnabled(false);
  af_->ui->btnKill->setEnabled(true);

  // presumption: all signals have to be emitted from another thread!
  connect(this, &WorkbenchAction::finished,
          af_, &AnalysisForm::onResultReady,
          Qt::QueuedConnection);

  connect(this, &WorkbenchAction::failed,
          af_, &AnalysisForm::onAnalysisError,
          Qt::QueuedConnection);

  connect(this, &WorkbenchAction::cancelled,
          af_, &AnalysisForm::onAnalysisCancelled,
          Qt::QueuedConnection);

  connect(this, &WorkbenchAction::statusMessage,
          [this](const QString& msg) { af_->statusMessage(msg); }
  );

}


WorkbenchAction::~WorkbenchAction()
{
  af_->ui->btnRun->setEnabled(true);
  af_->ui->btnKill->setEnabled(false);
}
