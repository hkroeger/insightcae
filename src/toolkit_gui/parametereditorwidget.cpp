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


ParameterEditorWidget::ParameterEditorWidget(insight::ParameterSet& pset, QWidget *parent,
                                             insight::ParameterSet_ValidatorPtr vali,
                                             insight::ParameterSet_VisualizerPtr viz)
: QSplitter(Qt::Horizontal, parent),
  parameters_(pset),
  vali_(vali),
  viz_(viz)
{
    ptree_=new QTreeWidget(this);
    addWidget(ptree_);
    inputContents_=new QWidget(this);
    addWidget(inputContents_);

    if (viz_)
    {
        context_=new QoccViewerContext;
        viewer_=new QoccViewWidget(context_->getContext(), this);
        addWidget(viewer_);
        modeltree_=new QModelTree(this);
        addWidget(modeltree_);

        connect(modeltree_, &QModelTree::show/*(QDisplayableModelTreeItem*)*/,
                viewer_, &QoccViewWidget::onShow);
        connect(modeltree_, &QModelTree::hide/*(QDisplayableModelTreeItem*)*/,
                viewer_, &QoccViewWidget::onHide);
        connect(modeltree_, &QModelTree::setDisplayMode/*(QDisplayableModelTreeItem*, AIS_DisplayMode)*/,
                viewer_, &QoccViewWidget::onSetDisplayMode);
        connect(modeltree_, &QModelTree::setColor/*(QDisplayableModelTreeItem*, Quantity_Color)*/,
                viewer_, &QoccViewWidget::onSetColor);
        connect(modeltree_, &QModelTree::setResolution/*(QDisplayableModelTreeItem*, double)*/,
                viewer_, &QoccViewWidget::onSetResolution);

    }
    else
    {
        context_=NULL;
        viewer_=NULL;
        modeltree_=NULL;
    }

    root_=new QTreeWidgetItem(0);
    root_->setText(0, "Parameters");
    ptree_->setColumnCount(2);
    ptree_->setHeaderLabels( QStringList() << "Parameter Name" << "Current Value" );
    ptree_->addTopLevelItem(root_);
    
    addWrapperToWidget(parameters_, root_, inputContents_, this);

    {
      QList<int> l;
      l << 3300 << 6600;
      if (viz_)
      {
        l << 6600 << 0;
      }
      setSizes(l);
    }
    
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

void ParameterEditorWidget::onUpdateVisualization()
{
    qDebug()<<"onUpdateVisualization";
    if (viz_)
    {
        qDebug()<<"exec update viz";
        viz_->update(parameters_);
        viz_->updateVisualizationElements(viewer_, modeltree_);
    }
}


void ParameterEditorWidget::onCheckValidity()
{
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
     
  QObject::connect(ptree_, &QTreeWidget::itemSelectionChanged, dp, &DirectoryParameterWrapper::onSelectionChanged);
  
  QObject::connect(this, &ParameterEditorWidget::apply, dp, &DirectoryParameterWrapper::onApply);
  QObject::connect(this, &ParameterEditorWidget::update, dp, &DirectoryParameterWrapper::onUpdate);
}
