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

// #include <QListWidget>
// #include <QListWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <map>
#include <string>




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




class HierarchyLevel
: public std::map<std::string, HierarchyLevel>
{
public:
    QTreeWidgetItem* parent_;
    
    HierarchyLevel(QTreeWidgetItem* parent)
    : parent_(parent)
    {}
    
    HierarchyLevel& sublevel(const std::string& entry)
    {
        iterator it = find(entry);
        if (it == end())
        {
            QTreeWidgetItem* newnode = new QTreeWidgetItem(parent_, QStringList() << entry.c_str());
            std::pair<iterator,bool> ret = insert(std::make_pair(entry, HierarchyLevel(newnode)));
            it = ret.first;
        }
        return it->second;
    }
};




void newAnalysisDlg::fillAnalysisList()
{

  HierarchyLevel toplevel ( new QTreeWidgetItem ( ui->treeWidget, QStringList() << "Available Analyses" ) );

  for ( insight::Analysis::FactoryTable::const_iterator i = insight::Analysis::factories_->begin();
        i != insight::Analysis::factories_->end(); i++ )
    {
      std::string analysisName = i->first;
      QStringList path = QString::fromStdString ( insight::Analysis::category ( analysisName ) ).split ( "/", QString::SkipEmptyParts );
      HierarchyLevel* parent = &toplevel;
      for ( QStringList::const_iterator pit = path.constBegin(); pit != path.constEnd(); ++pit )
        {
          parent = & ( parent->sublevel ( pit->toStdString() ) );
        }
      new QTreeWidgetItem ( parent->parent_, QStringList() << analysisName.c_str() );
    }
    
  ui->treeWidget->expandAll();
}




std::string newAnalysisDlg::getAnalysisName() const
{
    return ui->treeWidget->selectedItems()[0]->text(0).toStdString();
}
   
   
   
   
void newAnalysisDlg::done(int r)
{
  if ( r == QDialog::Accepted)
  {
    if (ui->treeWidget->selectedItems().size() == 1)
    {
        QTreeWidgetItem * curitem = ui->treeWidget->selectedItems()[0];
        if (curitem->childCount()==0) // is leaf?
        {
            QDialog::done(r);
        } else return;
    } else return;
  }
  
  QDialog::done(r);
}


