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
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenuBar>
#include <QMenu>

#include "isofcasebuilderwindow.h"
#include "insertedcaseelement.h"

#ifndef Q_MOC_RUN
#include "openfoam/remoteexecution.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh_templates.h"
#include "openfoam/snappyhexmesh.h"
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#endif

#include "base/qt5_helper.h"

#include "taskspoolermonitor.h"

using namespace insight;
using namespace boost;
using namespace rapidxml;




void isofCaseBuilderWindow::createCase
(
    bool skipBCs,
    const std::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles
)
{
  recreateOFCase ( ui->OFversion->currentText() );

  // insert case elements
  for ( int i=0; i < ui->selected_elements->count(); i++ )
    {
      InsertedCaseElement* cur
        = dynamic_cast<InsertedCaseElement*> ( ui->selected_elements->item ( i ) );
      if ( cur )
        {
          cur->insertElement ( *ofc_ );
        }
    }

  if (!restrictToFiles)
    ofc_->modifyFilesOnDiskBeforeDictCreation( casepath() );

  // insert BCs

  if (!boost::filesystem::exists(ofc_->boundaryDictPath(casepath())))
  {
      if (!skipBCs)
        QMessageBox::warning(this, "Warning", "No boundary dictionary present: skipping BC creation!");

      skipBCs=true;
  }

  insight::OFDictData::dict boundaryDict;
  if ( !skipBCs )
    {
      ofc_->parseBoundaryDict ( casepath(), boundaryDict );

      for ( int i=0; i < ui->patch_list->count(); i++ )
        {
          Patch* cur = dynamic_cast<Patch*> ( ui->patch_list->item ( i ) );
          if ( cur )
          {
 //           if ( boundaryDict.find(cur->patch_name()) != boundaryDict.end() )
            {
              cur->insertElement ( *ofc_, boundaryDict );
            }
          }
        }
    }
  if ( ofc_->getUnhandledPatches ( boundaryDict ).size() > 0 )
    {
      throw insight::Exception ( "Incorrect case setup: There are unhandled patches. Continuing would result in an invalid boundary definition." );
    }

  ofc_->createOnDisk ( casepath(), restrictToFiles );
  if ( !restrictToFiles ) ofc_->modifyCaseOnDisk ( casepath() );
}



void isofCaseBuilderWindow::onCreate()
{
  if (checkIfSaveNeeded())
  {

    if (ui->selected_elements->count() > 0)
    {

          if
          (
              QMessageBox::question
              (
                  this,
                  "Confirm",
                  str(format("Press OK to write the selected configuration into directory:\n%d!")
                      % casepath()).c_str(),
                  QMessageBox::Ok|QMessageBox::Cancel
              )
              ==
              QMessageBox::Ok
          )
          {
              createCase(false);
          }
     }
  }
}



void isofCaseBuilderWindow::onCreateNoBCs()
{
  if (checkIfSaveNeeded())
  {

    if (ui->selected_elements->count() > 0)
    {

          if
          (
              QMessageBox::question
              (
                  this,
                  "Confirm",
                  str(format("Press OK to write the selected configuration into directory:\n%d!")
                      % casepath()).c_str(),
                  QMessageBox::Ok|QMessageBox::Cancel
              )
              ==
              QMessageBox::Ok
          )
          {
              createCase(true);
          }
     }
  }
}



void createAndRunScript(const insight::OpenFOAMCase& ofc, const boost::filesystem::path& scriptpath, const std::string& prefix, const QString& script, std::function<QString()> default_generator)
{
  boost::filesystem::path fn = boost::filesystem::unique_path(scriptpath/(prefix+"-%%%%%.sh"));

  {
    std::ofstream f( fn.c_str() );
    f<<"#!/bin/bash\n";

    if (!script.isEmpty())
    {
      f<<script.toStdString();
    }
    else
    {
      QString sc = default_generator();
      f<<sc.toStdString();
    }
  }

  boost::filesystem::permissions(fn, boost::filesystem::perms(0755));

  ofc.executeCommand(scriptpath, fn.string());

//  boost::filesystem::remove(fn);
}


void isofCaseBuilderWindow::run(ExecutionStep begin_with)
{
  recreateOFCase( ui->OFversion->currentText() );

  boost::filesystem::path cp=casepath();

  boost::filesystem::path cfgfn = cp/"isofcasebuilder-run.iscb";
  boost::filesystem::path fn = cp/"isofcasebuilder-run.sh";
  boost::filesystem::path ts_socket = cp/"tsp.socket";

  if (boost::filesystem::exists(ts_socket))
  {
    insight::TaskSpoolerInterface ti(ts_socket);
    auto jobs=ti.jobs();
    if (jobs.hasRunningJobs() || jobs.hasQueuedJobs())
    {
      auto answer=QMessageBox::question
                  (
                    this,
                    QString::fromStdString("Execution server exists in "+cp.string()),
                    "There is a run server socket existing in the execution directory.\n"
                    "Please select yes to kill and restart the execution server. This will terminate any solver running on this case!\n",
                    QMessageBox::StandardButtons(QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel)
                  );

      if (answer!=QMessageBox::Yes) return;

      ti.cancelAllJobs();
    }
  }

  saveToFile(cfgfn);
  {
    std::ofstream f( fn.c_str() );
    f<<"#!/bin/bash\n";

    switch (begin_with)
    {
      case ExecutionStep_Clean:
        f<<"isofCleanCase -c\n";

      case ExecutionStep_Pre:

        if (script_pre_.isEmpty())
          f<<generateDefault_script_pre().toStdString()<<"\n\n";
        else
          f<<script_pre_.toStdString()<<"\n\n";

      case ExecutionStep_Mesh:

        f<<"isofCaseBuilder -sb "<<cfgfn<<"\n";

        if (script_mesh_.isEmpty())
          f<<generateDefault_script_mesh().toStdString()<<"\n\n";
        else
          f<<script_mesh_.toStdString()<<"\n\n";

      case ExecutionStep_Case:

        f<<"isofCaseBuilder -b "<<cfgfn<<"\n";

        if (script_case_.isEmpty())
          f<<generateDefault_script_case().toStdString()<<"\n\n";
        else
          f<<script_case_.toStdString()<<"\n\n";

    }

    boost::filesystem::permissions(fn, boost::filesystem::perms(0755));

  }

  ofc_->executeCommand(cp, "TS_SOCKET=\""+ts_socket.string()+"\" tsp "+fn.string());

  TaskSpoolerMonitorDialog *mon = new TaskSpoolerMonitorDialog(ts_socket, this);
  mon->show();
}





void isofCaseBuilderWindow::runAll()
{
  if (checkIfSaveNeeded())
  {

    if (ui->selected_elements->count() > 0)
    {

          if
          (
              QMessageBox::question
              (
                  this,
                  "Confirm",
                  str(format("Press OK to create and run the selected configuration in the directory:\n%d!")
                      % casepath()).c_str(),
                  QMessageBox::Ok|QMessageBox::Cancel
              )
              ==
              QMessageBox::Ok
          )
          {
              run(ExecutionStep_Pre);
          }
     }
  }
}


void isofCaseBuilderWindow::cleanAndRunAll()
{
  if (checkIfSaveNeeded())
  {

    if (ui->selected_elements->count() > 0)
    {

          if
          (
              QMessageBox::question
              (
                  this,
                  "Confirm",
                  str(format("Press OK to run the selected configuration in the directory:\n%d!\nNote: all existing previous case information will be deleted!")
                      % casepath()).c_str(),
                  QMessageBox::Ok|QMessageBox::Cancel
              )
              ==
              QMessageBox::Ok
          )
          {
              run(ExecutionStep_Clean);
          }
     }
  }
}

void isofCaseBuilderWindow::runMeshAndSolver()
{
  if (checkIfSaveNeeded())
  {

    if (ui->selected_elements->count() > 0)
    {

          if
          (
              QMessageBox::question
              (
                  this,
                  "Confirm",
                  str(format("Press OK to create and run the selected configuration in the directory:\n%d!")
                      % casepath()).c_str(),
                  QMessageBox::Ok|QMessageBox::Cancel
              )
              ==
              QMessageBox::Ok
          )
          {
              run(ExecutionStep_Mesh);
          }
     }
  }
}

void isofCaseBuilderWindow::runSolver()
{
  if (checkIfSaveNeeded())
  {

    if (ui->selected_elements->count() > 0)
    {

          if
          (
              QMessageBox::question
              (
                  this,
                  "Confirm",
                  str(format("Press OK to create and run the selected configuration in the directory:\n%d!")
                      % casepath()).c_str(),
                  QMessageBox::Ok|QMessageBox::Cancel
              )
              ==
              QMessageBox::Ok
          )
          {
              run(ExecutionStep_Case);
          }
     }
  }
}

