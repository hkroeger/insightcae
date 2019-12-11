
#include "analysisform.h"
#include "ui_analysisform.h"
#include <QMessageBox>

#include "base/analysis.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/remoteserverlist.h"

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/resultset.h"
#endif

#include <QMessageBox>


namespace bf = boost::filesystem;




AnalysisWorker::AnalysisWorker(const std::shared_ptr<insight::Analysis>& analysis)
: analysis_(analysis)
{}

void AnalysisWorker::doWork(insight::ProgressDisplayer* pd)
{
  try
  {
    insight::ResultSetPtr results = (*analysis_)(pd);
    emit resultReady( results );
  }
  catch (const std::exception& e)
  {
    if ( const auto* ie = dynamic_cast<const insight::Exception*>(&e) )
    {
      emit error( insight::Exception(*ie) );
    }
    else
    {
      emit error( insight::Exception(e.what()) );
    }
  }
  catch (boost::thread_interrupted i)
  {
    emit killed();
  }
  catch (...)
  {
    emit error(insight::Exception("An unhandled exception occurred in the analysis thread!"));
  }

}




void AnalysisForm::startLocalRun()
{
  if (!workerThread_)
  {

      if (results_)
      {
          QMessageBox msgBox;
          msgBox.setText("There is currently a result set in memory!");
          msgBox.setInformativeText("If you continue, the results will be deleted and the execution directory on disk will be removed (only if it was created). Continue?");
          msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
          msgBox.setDefaultButton(QMessageBox::Cancel);


          if (msgBox.exec()==QMessageBox::Yes)
          {
              results_.reset();
          } else {
              return;
          }
      }


      boost::filesystem::path exePath( ui->localDir->text().toStdString() );
      // it's ok, if path is empty in the following checks

      if (isOpenFOAMAnalysis_)
      {
        bool evalOnly = insight::OpenFOAMAnalysis::Parameters(parameters_).run.evaluateonly;

        if (boost::filesystem::exists(exePath / "constant" / "polyMesh" ))
        {
          if (!evalOnly)
          {
            QMessageBox msgBox;
            msgBox.setText("There is already an OpenFOAM case present in the execution directory \""
                           +QString(exePath.c_str())+"\"!");
            msgBox.setInformativeText(
                  "Depending on the state of the data, the behaviour will be as follows:<br><ul>"
                  "<li>the mesh exists (\"constant/polyMesh/\") and a time directory exists (e.g. \"0/\"): the solver will be restarted,</li>"
                  "<li>only the mesh exists (\"constant/polyMesh/\"): mesh creation will be skipped but the dictionaries will be recreated</li>"
                  "</ul><br>If you are unsure about the validity of the case data, please consider to click on \"Cancel\" and clean the case directory first (click on clean button on the right).<br>"
                  "<br>"
                  "Continue?"
                  );
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);


            if (msgBox.exec()!=QMessageBox::Yes)
            {
                return;
            }
          }
        }
        else
        {
          if (evalOnly)
          {
            QMessageBox msgBox;
            msgBox.setText("You have selected to run the evaluation only but there is no valid OpenFOAM case present in the execution directory \""
                           +QString(exePath.c_str())+"\"!");
            msgBox.setInformativeText(
                  "The subsequent step is likely to fail.<br>"
                  "Are you sure, that you want to continue?"
                  );
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);


            if (msgBox.exec()!=QMessageBox::Yes)
            {
                return;
            }
          }
        }
      }

      emit apply(); // apply all changes into parameter set

      analysis_.reset( insight::Analysis::lookup(analysisName_, parameters_, exePath) );

      progdisp_->reset();

      ui->tabWidget->setCurrentWidget(ui->runTab);

      workerThread_.reset(new boost::thread( [&]() {
        AnalysisWorker worker(analysis_);

        connect(&worker, &AnalysisWorker::resultReady,
                this, &AnalysisForm::onResultReady,
                Qt::QueuedConnection);
        connect(&worker, &AnalysisWorker::error,
                this, &AnalysisForm::onAnalysisErrorOccurred,
                Qt::QueuedConnection);
        connect(&worker, &AnalysisWorker::killed,
                this, &AnalysisForm::onAnalysisKilled,
                Qt::QueuedConnection);

        worker.doWork(progdisp_);
      }
      ));



  }
}




void AnalysisForm::stopLocalRun()
{
  if (workerThread_)
  {
    workerThread_->interrupt();
  }
}
