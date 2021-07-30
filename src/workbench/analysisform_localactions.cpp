
#include "analysisform.h"
#include "ui_analysisform.h"
#include <QMessageBox>
#include <QDebug>

#include "base/analysis.h"
#include "base/remoteserverlist.h"
#include "openfoam/ofes.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamanalysis.h"

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/resultset.h"
#endif

#include <QMessageBox>
#include <QFileDialog>
#include "qexecutionenvironmentdialog.h"

#include "localrun.h"


namespace fs = boost::filesystem;


void AnalysisForm::connectLocalActions()
{
  // ====================================================================================
  // ======== edit working directory

  connect(ui->btnSetExecutionEnvironment, &QPushButton::clicked, this,
          [&]()
          {
            insight::RemoteLocation *rl = nullptr;
            if (auto* rec = remoteExecutionConfiguration())
              rl=&rec->location();

            QExecutionEnvironmentDialog dlg(
                  localCaseDirectory_.get(),
                  rl,
                  this );

            if (dlg.exec() == QDialog::Accepted)
            {
              remoteExeConfigWasEdited_ = true;

              if (remoteExecutionConfiguration_)
                delete remoteExecutionConfiguration_;
              localCaseDirectory_.reset();

              auto lwd = dlg.localDirectory();

              insight::dbg()<<"localDirectory="<<lwd<<std::endl;

              if (lwd.empty())
              {
                localCaseDirectory_.reset(
                      new IQCaseDirectoryState(
                        this, boost::filesystem::path(), false) );
              }
              else
              {
                localCaseDirectory_.reset(
                      new IQCaseDirectoryState(
                        this, lwd ) );
              }

              if (localCaseDirectory_ && dlg.remoteLocation())
              {
                if (remoteExecutionConfiguration_)
                  delete remoteExecutionConfiguration_;

                remoteExecutionConfiguration_ =
                      IQRemoteExecutionState::New<IQWorkbenchRemoteExecutionState>(
                        this,
                        *dlg.remoteLocation() );
              }
            }
          }
  );

}


boost::filesystem::path AnalysisForm::localCaseDirectory() const
{
  if (!localCaseDirectory_)
  {
    const_cast<std::unique_ptr<IQCaseDirectoryState>&>
        (localCaseDirectory_).reset(
          new IQCaseDirectoryState(
            const_cast<AnalysisForm*>(this), false ) );
  }
  return *localCaseDirectory_;
}


void AnalysisForm::startLocalRun()
{
  if (isOpenFOAMAnalysis_)
  {
    bool evalOnly = insight::OpenFOAMAnalysis::Parameters(parameters()).run.evaluateonly;

    if ( boost::filesystem::exists( localCaseDirectory() / "constant" / "polyMesh" ) )
    {
      if (!evalOnly)
      {
        QMessageBox msgBox;
        msgBox.setText("There is already an OpenFOAM case present in the execution directory \""
                       +QString::fromStdString(localCaseDirectory().string())+"\"!");
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
                       +QString::fromStdString(localCaseDirectory().string())+"\"!");
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

//  Q_EMIT apply(); // apply all changes into parameter set

  currentWorkbenchAction_.reset(new LocalRun(this));
}

