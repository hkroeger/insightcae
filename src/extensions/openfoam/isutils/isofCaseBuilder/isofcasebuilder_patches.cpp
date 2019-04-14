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



ParameterSet& isofCaseBuilderWindow::BCParameters(const std::string& patchName)
{
    QList<QListWidgetItem *>  items
      = ui->patch_list->findItems(QString(patchName.c_str()), Qt::MatchStartsWith);
    if (items.size()<1)
        throw insight::Exception
        (
            "Error: patch \""+patchName+"\" was not found!"
        );
    if (items.size()>1)
        throw insight::Exception
        (
            "Error: patch name \""+patchName+"\" matches multiple entries!"
        );

    Patch* cur = dynamic_cast<Patch*>(items[0]);
    if (!cur)
        throw insight::Exception
        (
            "Error: Requested patch \""+patchName+"\" has no valid configuration!"
        );

    return cur->parameters();
}



void isofCaseBuilderWindow::onParseBF()
{
    insight::OFDictData::dict boundaryDict;

    ofc_->parseBoundaryDict(casepath(), boundaryDict);
    ui->patch_list->clear();
    new DefaultPatch(ui->patch_list);
    for (const OFDictData::dict::value_type& bde: boundaryDict)
    {
//         unhandledPatches.insert(bde.first);
        new Patch(ui->patch_list, bde.first);
    }
}






void isofCaseBuilderWindow::onPatchSelectionChanged()
{
    Patch* cur = dynamic_cast<Patch*>(ui->patch_list->currentItem());
    if (cur)
    {
        if (bc_ped_)
        {
          last_bc_pe_state_ = bc_ped_->saveState();
          bc_ped_->deleteLater();
        }
        bc_ped_ = new ParameterEditorWidget(cur->parameters(), ui->bc_parameter_editor);
        bc_pe_layout_->addWidget(bc_ped_);

        if (!last_bc_pe_state_.isEmpty())
        {
          bc_ped_->restoreState(last_bc_pe_state_);
        }
    //     ui->parameter_editor->setCentralWidget(ped_);

    //     ParameterSet emptyps;
    //     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
    }
}



void isofCaseBuilderWindow::onAssignBC()
{
    QListWidgetItem *curbctype=ui->bc_element_list->currentItem();
    Patch *curpatch = dynamic_cast<Patch*>(ui->patch_list->currentItem());
    if (curbctype && curpatch)
    {
        std::string type_name = curbctype->text().toStdString();
        curpatch->set_bc_type(type_name);
        onPatchSelectionChanged();
    }
}



void isofCaseBuilderWindow::onAddPatchManually()
{
    QString pname = QInputDialog::getText(this, "Insert Patch", "Enter patch name:");
    if (!pname.isEmpty())
    {
        new Patch(ui->patch_list, pname.toStdString());
    }
}




void isofCaseBuilderWindow::onRemovePatch()
{
  auto* ci=ui->patch_list->currentItem();
  Patch *curpatch = dynamic_cast<Patch*>(ci);
  if (curpatch)
  {
    if (dynamic_cast<DefaultPatch*>(ci))
    {
      QMessageBox::critical(this, "Invalid operation", "The default patch entry cannot be removed!");
    }
    else
    {
      ui->patch_list->removeItemWidget(ci);
      delete ci;
    }
  }
}




void isofCaseBuilderWindow::onRenamePatch()
{
  auto* ci=ui->patch_list->currentItem();
  Patch *curpatch = dynamic_cast<Patch*>(ci);
  if (curpatch)
  {
    if (dynamic_cast<DefaultPatch*>(ci))
    {
      QMessageBox::critical(this, "Invalid operation", "The default patch entry cannot be renamed!");
    }
    else
    {
      QString pname = QInputDialog::getText(this, "Rename Patch", "Enter new patch name:", QLineEdit::Normal, curpatch->text());
      if (!pname.isEmpty())
      {
          curpatch->set_patch_name(pname);
      }
    }
  }
}




void isofCaseBuilderWindow::onResetPatchDef()
{
  ui->patch_list->clear();
  new DefaultPatch(ui->patch_list);
}
