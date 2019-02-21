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
                                             insight::ParameterSet_VisualizerPtr viz,
                                             QoccViewWidget *viewwidget,
                                             QModelTree *modeltree)
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
        if (!viewwidget)
        {
          QoccViewerContext *context=new QoccViewerContext(this);
          viewer_=new QoccViewWidget(this, context->getContext());
          addWidget(viewer_);
        }
        else
        {
          viewer_=viewwidget;
        }

        if (!modeltree)
        {
          modeltree_=new QModelTree(this);
          addWidget(modeltree_);
        }
        else
        {
          modeltree_=modeltree;
        }

        if ( (!viewwidget) || (!modeltree) ) // one of them was created here
        {
          viewer_->connectModelTree(modeltree_);
        }
    }
    else
    {
        viewer_ = nullptr;
        modeltree_ = nullptr;
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
      if (viz_ && !viewwidget && !modeltree)
      {
        l << 6600 << 0;
      }
      setSizes(l);
    }
    
    ptree_->expandAll();
    ptree_->resizeColumnToContents(0);
    ptree_->resizeColumnToContents(1);
    ptree_->setContextMenuPolicy(Qt::CustomContextMenu);

    onCheckValidity();
    onUpdateVisualization();

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


