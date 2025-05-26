/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
//  *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

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
#include "base/translations.h"

#include <QMessageBox>
#include <QFileDialog>

#include "localrun.h"


namespace fs = boost::filesystem;


void AnalysisForm::connectLocalActions()
{
  // ====================================================================================
  // ======== edit working directory

  connect(ui->btnSetExecutionEnvironment, &QPushButton::clicked,
          this, [&]()
          {
              showSetupExecutionEnvironmentDialog();
          }
          );

}



void AnalysisForm::startLocalRun()
{
  if (isOpenFOAMAnalysis_)
  {
    bool evalOnly = insight::OpenFOAMAnalysis::Parameters(
                          parameters()).run.evaluateonly;

    if ( boost::filesystem::exists(
            localCaseDirectory() / "constant" / "polyMesh" ) )
    {
      if (!evalOnly)
      {
        QMessageBox msgBox;
          msgBox.setText(
            QString(_("There is already an OpenFOAM case present in the execution directory \"%1\"!")).arg(
                QString::fromStdString(localCaseDirectory().string())));
        msgBox.setInformativeText(
              _("Depending on the state of the data, the behaviour will be as follows:<br><ul>"
              "<li>the mesh exists (\"constant/polyMesh/\") and a time directory exists (e.g. \"0/\"): "
                "the solver will be restarted,</li>"
              "<li>only the mesh exists (\"constant/polyMesh/\"): "
                "mesh creation will be skipped but the dictionaries will be recreated</li>"
              "</ul><br>If you are unsure about the validity of the case data, "
                "please consider to click on \"Cancel\" and "
                "clean the case directory first (click on clean button on the right).<br>"
              "<br>"
                "Continue?")
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
        msgBox.setText(
            QString(_("You have selected to run the evaluation only but there is no valid OpenFOAM case present in the execution directory \"%1\"!"))
                .arg( QString::fromStdString(localCaseDirectory().string())) );
        msgBox.setInformativeText(
              _("The subsequent step is likely to fail.<br>"
              "Are you sure, that you want to continue?")
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




bool AnalysisForm::isOpenFOAMAnalysis() const
{
    return isOpenFOAMAnalysis_;
}

insight::OperatingSystemSet
AnalysisForm::compatibleOperatingSystems() const
{
    return insight::Analysis::compatibleOperatingSystemFunctions()(analysisName_);
}


