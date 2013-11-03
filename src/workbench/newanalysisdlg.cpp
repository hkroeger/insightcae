/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  hannes <email>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "newanalysisdlg.h"
#include "ui_newanalysisdlg.h"

#include "base/analysis.h"

#include <QListWidget>
#include <QListWidgetItem>

newAnalysisDlg::newAnalysisDlg(QWidget* parent)
: QDialog(parent)
{
  ui=new Ui::newAnalysisDlg();
  ui->setupUi(this);

  this->setWindowTitle("Create New Analysis");
  
  connect(this->ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(this->ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  
  fillAnalysisList();
}

newAnalysisDlg::~newAnalysisDlg()
{
  delete ui;
}

void newAnalysisDlg::fillAnalysisList()
{
  for (insight::Analysis::FactoryTable::const_iterator i = insight::Analysis::factories_.begin();
       i != insight::Analysis::factories_.end(); i++)
  {
    new QListWidgetItem(i->first.c_str(), ui->listWidget);
  }
}

std::string newAnalysisDlg::getAnalysisName() const
{
  return ui->listWidget->selectedItems()[0]->text().toStdString();
}
    
void newAnalysisDlg::done(int r)
{
  if ( r == QDialog::Accepted)
  {
    if (ui->listWidget->selectedItems().size() == 1)
      QDialog::done(r);
    else return;
  }
  
  QDialog::done(r);
}


