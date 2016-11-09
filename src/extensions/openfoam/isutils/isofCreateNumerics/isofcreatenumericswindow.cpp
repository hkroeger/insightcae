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


#include <QListWidgetItem>
#include <QMessageBox>

#include "isofcreatenumericswindow.h"

#include "openfoam/openfoamcase.h"

using namespace insight;
using namespace boost;


isofCreateNumericsWindow::isofCreateNumericsWindow()
: QDialog(), ped_(NULL)
{
    ui = new Ui::isofCreateNumericsWindow;
    ui->setupUi(this);
    
    for (insight::OpenFOAMCaseElement::FactoryTable::const_iterator i = insight::OpenFOAMCaseElement::factories_->begin();
        i != insight::OpenFOAMCaseElement::factories_->end(); i++)
    {
        new QListWidgetItem(i->first.c_str(), ui->listWidget);
    }
    
    QObject::connect 
    ( 
        ui->listWidget, SIGNAL ( itemSelectionChanged() ),
        this, SLOT ( onItemSelectionChanged() ) 
    );
    
    ofc_.reset(new OpenFOAMCase(OFEs::get("OF23x")));
}




isofCreateNumericsWindow::~isofCreateNumericsWindow()
{
}




void isofCreateNumericsWindow::onItemSelectionChanged()
{
    QListWidgetItem* cur = ui->listWidget->currentItem();
    std::string num_name = cur->text().toStdString();
    
    if (ped_) ped_->deleteLater();
    parameters_=insight::OpenFOAMCaseElement::defaultParameters(num_name);
    ped_ = new ParameterEditorWidget(parameters_, ui->splitter);
    ui->splitter->insertWidget(1, ped_);
    
//     ParameterSet emptyps;
//     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
}


void isofCreateNumericsWindow::done(int r)
{
  if ( r == QDialog::Accepted)
  {
        QListWidgetItem* cur = ui->listWidget->currentItem();
        if (cur)
        {
            std::string num_name = cur->text().toStdString();
            boost::filesystem::path cwd = boost::filesystem::current_path();
            if 
            (
                QMessageBox::question
                (
                    this, 
                    "Confirm", 
                    str(format("Press OK to write configuration for OF solver %s into current directory %d!")
                        % num_name % cwd).c_str(),
                    QMessageBox::Ok|QMessageBox::Cancel
                ) 
                == 
                QMessageBox::Ok
            )
            {
                ofc_->insert( OpenFOAMCaseElement::lookup(num_name, *ofc_, parameters_) );
                ofc_->createOnDisk(cwd);
                ofc_->modifyCaseOnDisk(cwd);
            }
            else
            {
                return;
            }
        }
        else
        {
            return;
        }
  }
  
  QDialog::done(r);
}
