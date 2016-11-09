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

#include "isofcasebuilderwindow.h"



using namespace insight;
using namespace boost;




InsertedCaseElement::InsertedCaseElement(QListWidget* parent, const std::string& type_name)
: QListWidgetItem(parent), type_name_(type_name)
{
    curp_ = insight::OpenFOAMCaseElement::defaultParameters(type_name);
    setText(type_name.c_str());
}


void InsertedCaseElement::insertElement(insight::OpenFOAMCase& c) const
{
    c.insert(insight::OpenFOAMCaseElement::lookup(type_name_, c, curp_));
}


isofCaseBuilderWindow::isofCaseBuilderWindow()
: QDialog(), ped_(NULL)
{
    // setup layout
    ui = new Ui::isofCaseBuilderWindow;
    ui->setupUi(this);
    pe_layout_ = new QHBoxLayout(ui->parameter_editor);
    
    // populate list of available case elements
    for (insight::OpenFOAMCaseElement::FactoryTable::const_iterator i = insight::OpenFOAMCaseElement::factories_->begin();
        i != insight::OpenFOAMCaseElement::factories_->end(); i++)
    {
        new QListWidgetItem(i->first.c_str(), ui->available_elements);
    }
    
    QObject::connect 
    ( 
        ui->selected_elements, SIGNAL ( itemSelectionChanged() ),
        this, SLOT ( onItemSelectionChanged() ) 
    );
    
    connect(ui->add_btn, SIGNAL(clicked()), this, SLOT(onAddElement()));
    connect(ui->remove_btn, SIGNAL(clicked()), this, SLOT(onRemoveElement()));
    
    ofc_.reset(new OpenFOAMCase(OFEs::get("OF23x")));
}




isofCaseBuilderWindow::~isofCaseBuilderWindow()
{
}




void isofCaseBuilderWindow::onItemSelectionChanged()
{
    InsertedCaseElement* cur = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->currentItem());
    if (cur)
    {
        if (ped_) ped_->deleteLater();
        ped_ = new ParameterEditorWidget(cur->parameters(), ui->parameter_editor);
        pe_layout_->addWidget(ped_);
    //     ui->parameter_editor->setCentralWidget(ped_);
        
    //     ParameterSet emptyps;
    //     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
    }
}


void isofCaseBuilderWindow::onAddElement()
{
    QListWidgetItem* cur = ui->available_elements->currentItem();
    if (cur)
    {
        std::string type_name = cur->text().toStdString();
        InsertedCaseElement* ice = new InsertedCaseElement(ui->selected_elements, type_name);
    }
}


void isofCaseBuilderWindow::onRemoveElement()
{
    QListWidgetItem* cur = ui->selected_elements->currentItem();
    if (cur)
    {
        delete cur;
    }
}


void isofCaseBuilderWindow::done(int r)
{
  if ( r == QDialog::Accepted)
  {
      if (ui->selected_elements->count() > 0)
      {
            boost::filesystem::path cwd = boost::filesystem::current_path();
            if 
            (
                QMessageBox::question
                (
                    this, 
                    "Confirm", 
                    str(format("Press OK to write the selected configuration into current directory %d!")
                        % cwd).c_str(),
                    QMessageBox::Ok|QMessageBox::Cancel
                ) 
                == 
                QMessageBox::Ok
            )
            {
                for (int i=0; i < ui->selected_elements->count(); i++)
                {
                    InsertedCaseElement* cur = dynamic_cast<InsertedCaseElement*>(ui->selected_elements->item(i));
                    if (cur)
                    {
                        cur->insertElement(*ofc_);
                    }
                }
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
