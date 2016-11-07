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

#include "isofcreatenumericswindow.h"

#include "openfoam/openfoamcase.h"

using namespace insight;


isofCreateNumericsWindow::isofCreateNumericsWindow()
: QDialog(), ped_(NULL)
{
    ui = new Ui::isofCreateNumericsWindow;
    ui->setupUi(this);
    
    for (insight::FVNumerics::FactoryTable::const_iterator i = insight::FVNumerics::factories_->begin();
        i != insight::FVNumerics::factories_->end(); i++)
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
    parameters_=insight::FVNumerics::defaultParameters(num_name);
    ped_ = new ParameterEditorWidget(parameters_, ui->splitter);
    ui->splitter->insertWidget(1, ped_);
    
//     ParameterSet emptyps;
//     numerics_.reset(insight::FVNumerics::lookup(num_name, FVNumericsParameters(*ofc_, emptyps)));
}
