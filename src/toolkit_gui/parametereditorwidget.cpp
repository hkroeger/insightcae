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

#include "parametersetvisualizer.h"
#include "parametereditorwidget.h"
#include "parameterwrapper.h"


#include <QSplitter>

ParameterEditorWidget::ParameterEditorWidget
(
  insight::ParameterSet& pset,
  const insight::ParameterSet& default_pset,
  QWidget *parent,
  insight::ParameterSet_VisualizerPtr viz,
  insight::ParameterSet_ValidatorPtr vali,
  ParameterSetDisplay* display
)
: QSplitter(Qt::Horizontal, parent),
  parameters_(pset),
  defaultParameters_(default_pset),
  vali_(vali),
  viz_(std::dynamic_pointer_cast<insight::CAD_ParameterSet_Visualizer>(viz))
{
    ptree_=new QTreeWidget(this);
    addWidget(ptree_);
    inputContents_=new QWidget(this);
    addWidget(inputContents_);

    if (viz_)
    {
      // there is a visualization generator available

      if (!display)
      {
        // no existing displayer supplied; create one
        QoccViewerContext *context=new QoccViewerContext(this);
        QoccViewWidget *viewer=new QoccViewWidget(this, context->getContext());
        addWidget(viewer);

        QModelTree* modeltree =new QModelTree(this);
        addWidget(modeltree);

        viewer->connectModelTree(modeltree);

        display_=new ParameterSetDisplay(this, viewer, modeltree);
        display_->registerVisualizer(viz_);
      }
      else
      {
        // use supplied displayer
        display_=display;
      }
    }
    else
    {
      display_ = nullptr;
    }

    root_=new QTreeWidgetItem(0);
    root_->setText(0, "Parameters");
    ptree_->setColumnCount(2);
    ptree_->setHeaderLabels( QStringList() << "Parameter Name" << "Current Value" );
    ptree_->addTopLevelItem(root_);
    
    addWrapperToWidget(parameters_, defaultParameters_, root_, inputContents_, this);

    {
      QList<int> l;
      l << 3300 << 6600;
      if (viz_ && !display_)
      {
        l.append(6600);
        l.append(0);
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
  doUpdateVisualization();
}

void ParameterEditorWidget::doUpdateVisualization()
{
  if (viz_)
  {
      viz_->update(parameters_);
  }
}

void ParameterEditorWidget::onParameterSetChanged()
{
  emit parameterSetChanged();
  doUpdateVisualization();
}


void ParameterEditorWidget::insertParameter(const QString& name, insight::Parameter& parameter, const insight::Parameter& defaultParameter)
{
    DirectoryParameterWrapper *dp =
        new DirectoryParameterWrapper
    (
        root_,
        name,
        parameter,
        defaultParameter,
        inputContents_,
        this
    );
     
  QObject::connect(ptree_, &QTreeWidget::itemSelectionChanged, dp, &DirectoryParameterWrapper::onSelectionChanged);
  
  QObject::connect(this, &ParameterEditorWidget::apply, dp, &DirectoryParameterWrapper::onApply);
  QObject::connect(this, &ParameterEditorWidget::update, dp, &DirectoryParameterWrapper::onUpdate);
}



ParameterSetDisplay::ParameterSetDisplay
(
    QObject* parent,
    QoccViewWidget* viewer,
    QModelTree* modeltree
)
  : QObject(parent),
    viewer_(viewer),
    modeltree_(modeltree)
{
}

void ParameterSetDisplay::registerVisualizer(std::shared_ptr<insight::CAD_ParameterSet_Visualizer> viz)
{
  if (viz)
  {
    if (visualizers_.find(viz)==visualizers_.end())
    {
      visualizers_.insert(viz);
      connect(viz.get(), &insight::CAD_ParameterSet_Visualizer::GUINeedsUpdate,
              this, &ParameterSetDisplay::onUpdateVisualization);
    }
  }
}

void ParameterSetDisplay::deregisterVisualizer(std::shared_ptr<insight::CAD_ParameterSet_Visualizer> viz)
{
  if (viz)
  {
    disconnect(viz.get(), &insight::CAD_ParameterSet_Visualizer::GUINeedsUpdate,
               this, &ParameterSetDisplay::onUpdateVisualization);
    visualizers_.erase(viz);
  }
}

void ParameterSetDisplay::onUpdateVisualization()
{
  insight::cad::cache.initRebuild();

  insight::CAD_ParameterSet_Visualizer::UsageTracker ut(modeltree_);

  for (auto& vz: visualizers_)
  {
    vz->recreateVisualizationElements(&ut);
  }

  ut.cleanupModelTree();

  insight::cad::cache.finishRebuild();
}
