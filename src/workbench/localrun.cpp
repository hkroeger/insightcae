#include "localrun.h"
#include "analysisform.h"
#include "ui_analysisform.h"
#include "progressrelay.h"

#include <QMessageBox>




LocalRun::LocalRun(AnalysisForm *af)
  : WorkbenchAction(af)
{

  // lock UI
  af_->ui->label_2->setEnabled(false);
  af_->ui->localDir->setEnabled(false);
  af_->ui->cbDontKeepExeDir->setEnabled(false);
  af_->ui->btnSelectExecDir->setEnabled(false);
  af_->ui->cbRemoteRun->setEnabled(false);
  af_->ui->lbRR1->setEnabled(false);
  af_->ui->hostList->setEnabled(false);
  af_->ui->portNum->setEnabled(false);
  af_->ui->btnResume->setEnabled(false);
  af_->ui->btnDisconnect->setEnabled(false);
  af_->ui->lbRR2->setEnabled(false);
  af_->ui->remoteDir->setEnabled(false);
  af_->ui->btnSelectRemoteDir->setEnabled(false);
  af_->ui->btnUpload->setEnabled(false);
  af_->ui->btnDownload->setEnabled(false);
  af_->ui->btnRemoveRemote->setEnabled(false);

  analysis_.reset( insight::Analysis::lookup(af_->analysisName_, af_->parameters_,
                                             boost::filesystem::path(af_->ui->localDir->text().toStdString())) );

  af_->progdisp_->reset();
  af_->ui->tabWidget->setCurrentWidget(af_->ui->runTab);

  workerThread_ = boost::thread(
        [&]()
        {
          try
          {
            ProgressRelay pr;
            connect(&pr, &ProgressRelay::progressUpdate,
                    this, &WorkbenchAction::analysisProgressUpdate);

            insight::ResultSetPtr results = (*analysis_)( pr );

            Q_EMIT finished( results );
          }
          catch (...) { exceptionEmitter(); }
        }
  );

}

LocalRun::~LocalRun()
{
  workerThread_.join();

  // unlock UI
  af_->ui->label_2->setEnabled(true);
  af_->ui->localDir->setEnabled(true);
  af_->ui->cbDontKeepExeDir->setEnabled(true);
  af_->ui->btnSelectExecDir->setEnabled(true);

  af_->ui->cbRemoteRun->setEnabled(true);
  af_->recheckButtonAvailability();
}

void LocalRun::onCancel()
{
  workerThread_.interrupt();
}
