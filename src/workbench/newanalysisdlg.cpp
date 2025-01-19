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

#include "newanalysisdlg.h"
#include "ui_newanalysisdlg.h"

#include "base/analysis.h"
#include "base/translations.h"

// #include <QListWidget>
// #include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <map>
#include <string>

#include "workbenchwindow.h"



newAnalysisDlg::newAnalysisDlg(WorkbenchMainWindow* parent)
: QDialog(parent)
{
  ui=new Ui::newAnalysisDlg();
  ui->setupUi(this);

  this->setWindowTitle(_("Create New Analysis"));
  
  auto cancelBtn=new QPushButton("Cancel");
  ui->widget->replaceLoadButton(cancelBtn);

  connect(cancelBtn, &QPushButton::clicked,
          this, &QDialog::reject);

  connect(ui->widget, &NewAnalysisForm::createAnalysis,
          parent, &WorkbenchMainWindow::newAnalysis);
  connect(ui->widget, &NewAnalysisForm::createAnalysis,
          this, &QDialog::accept);
}




newAnalysisDlg::~newAnalysisDlg()
{
  delete ui;
}



