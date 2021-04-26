
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

#include "localrun.h"


namespace fs = boost::filesystem;


void AnalysisForm::connectLocalActions()
{
  // ====================================================================================
  // ======== working directory


  connect(ui->btnSelectWorkingDirectory, &QPushButton::clicked,
           [&]()
           {
             QString dir = QFileDialog::getExistingDirectory(
                   this,
                   "Please select working directory",
                   ui->leWorkingDirectory->text()
                   );
             if (!dir.isEmpty())
             {
               ui->leWorkingDirectory->setText(dir); // checks will be performed in lineEdit handler
             }
           }
   );


  connect(ui->leWorkingDirectory, &QLineEdit::textEdited,
          this, &AnalysisForm::workingDirectoryEdited);

  connect(ui->cbRemoveWorkingDirectory, &QCheckBox::toggled,
          [&](bool checked)
          {
            bool keep = !checked;

            if (caseDirectory_)
            {
              if ( !keep && (caseDirectory_->isExistingAndNotEmpty() && caseDirectory_->keep()) )
              {
                auto answer = QMessageBox::warning(
                      this,
                      "Attention!",

                      "You selected an <b>existing directory</b> as working directory and\n"
                      "you requested <b>removal of that directory</b> after the analysis is finished!\n"
                      "All data in the directory<br>"
                      "<b>"+QString::fromStdString(caseDirectory_->string())+"</b><br>"
                      "will be deleted after the analysis!"
                      "<br>"
                      "Do you really want to continue?",

                      QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel,
                      QMessageBox::No
                      );

                if (answer!=QMessageBox::Yes)
                {
                  ui->cbRemoveWorkingDirectory->setChecked( Qt::Unchecked );
                  return;
                }
              }
              caseDirectory_->setKeep(keep);
            }
          }
  );
}


void AnalysisForm::workingDirectoryEdited(const QString &qnwd)
{
  fs::path nwd=qnwd.toStdString();

  if ( !nwd.empty() && fs::exists(nwd) && fs::is_directory(nwd) )
  {
    caseDirectory_.reset(); // delete first
    caseDirectory_.reset(new QCaseDirectory( this, nwd, true ));
  }
  else
  {
    caseDirectory_.reset();
  }

  if (nwd.empty())
  {
    ui->cbRemoveWorkingDirectory->setChecked( Qt::Checked );
  }
  else if (fs::exists(nwd))
  {
    ui->cbRemoveWorkingDirectory->setChecked( Qt::Unchecked );
  }
}


bool AnalysisForm::ensureWorkingDirectoryExistence()
{
  if (!caseDirectory_)
  {
    qDebug()<<"RESET";
    bool keep = !ui->cbRemoveWorkingDirectory->isChecked();
    fs::path nwd = ui->leWorkingDirectory->text().toStdString();

    caseDirectory_.reset(new QCaseDirectory(this, nwd, keep));

    ui->leWorkingDirectory->setText(QString::fromStdString(caseDirectory_->string()));
  }

  return true;
}




void AnalysisForm::startLocalRun()
{
  if (isOpenFOAMAnalysis_)
  {
    bool evalOnly = insight::OpenFOAMAnalysis::Parameters(parameters()).run.evaluateonly;

    if (boost::filesystem::exists(*caseDirectory_ / "constant" / "polyMesh" ))
    {
      if (!evalOnly)
      {
        QMessageBox msgBox;
        msgBox.setText("There is already an OpenFOAM case present in the execution directory \""
                       +QString::fromStdString(caseDirectory_->string())+"\"!");
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
                       +QString::fromStdString(caseDirectory_->string())+"\"!");
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

