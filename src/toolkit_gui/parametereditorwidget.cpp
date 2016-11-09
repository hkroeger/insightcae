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

#include "parametereditorwidget.h"
#include "parameterwrapper.h"

#include <QSplitter>


ParameterEditorWidget::ParameterEditorWidget(insight::ParameterSet& pset, QWidget *parent)
: QSplitter(Qt::Horizontal, parent),
  parameters_(pset)
{
    ptree_=new QTreeWidget(this);
    addWidget(ptree_);
    inputContents_=new QWidget(this);
    addWidget(inputContents_);
    
    root_=new QTreeWidgetItem(0);
    root_->setText(0, "Parameters");
    ptree_->setColumnCount(2);
    ptree_->setHeaderLabels( QStringList() << "Parameter Name" << "Current Value" );
    ptree_->addTopLevelItem(root_);
    
    addWrapperToWidget(parameters_, root_, inputContents_, this);
    
    ptree_->expandAll();
    ptree_->resizeColumnToContents(0);
    ptree_->resizeColumnToContents(1);
    ptree_->setContextMenuPolicy(Qt::CustomContextMenu);

}

void ParameterEditorWidget::onApply()
{
    emit apply();
}

void ParameterEditorWidget::onUpdate()
{
    emit update();
}

void ParameterEditorWidget::insertParameter(const QString& name, insight::Parameter& parameter)
{
    DirectoryParameterWrapper *dp =
        new DirectoryParameterWrapper
    (
        root_,
        name,
        parameter,
        inputContents_,
        this
    );
     
  QObject::connect(ptree_, SIGNAL(itemSelectionChanged()), dp, SLOT(onSelectionChanged()));
  
  QObject::connect(this, SIGNAL(apply()), dp, SLOT(onApply()));
  QObject::connect(this, SIGNAL(update()), dp, SLOT(onUpdate()));
}
