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
#include <QLabel>


ParameterTreeWidget::ParameterTreeWidget(QWidget* p)
  : QTreeWidget(p)
{
}

void ParameterTreeWidget::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  QStyleOptionViewItem newOption(option);

  if ( QTreeWidgetItem * it = this->itemFromIndex(index) )
  {
    QBrush c = it->background(0);
    if ( c.color() != Qt::black )
    {
      painter->fillRect(option.rect, c);
      newOption.palette.setBrush( QPalette::Base, c);
      newOption.palette.setBrush( QPalette::AlternateBase, c);
    }
  }

  QTreeWidget::drawRow(painter, newOption, index);
}

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
  {
    QWidget *w=new QWidget(this);
    QVBoxLayout *l=new QVBoxLayout(w);
    w->setLayout(l);
    ptree_=new ParameterTreeWidget(w);
    l->addWidget(ptree_);
    QLabel *hints=new QLabel(w);
    hints->setStyleSheet("font: 8pt;");
    hints->setText(
          "Please adapt the parameters in the list above.\n"
          "Yellow background: need to revised for each case.\n"
          "Light gray: can usually be left on default values."
          );
    l->addWidget(hints);
    addWidget(w);
  }

    inputContents_=new QWidget(this);
    addWidget(inputContents_);

    if (viz_)
    {
      // there is a visualization generator available

      if (!display)
      {
        // no existing displayer supplied; create one
        QoccViewerContext *context=new QoccViewerContext(this);
        QWidget *w=new QWidget(this);
        QVBoxLayout *l=new QVBoxLayout(w);
        QoccViewWidget *viewer=new QoccViewWidget(w, context->getContext());
        viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        QLabel *hints=new QLabel(w);
        hints->setStyleSheet("font: 8pt;");
        hints->setWordWrap(true);
        hints->setText(
              "<b>Rotate</b>: Alt + Mouse Move, <b>Pan</b>: Shift + Mouse Move, <b>Zoom</b>: Ctrl + Mouse Move or Mouse Wheel, "
              "<b>Context Menu</b>: Right click on object or canvas."
              );
        l->addWidget(viewer);
        l->addWidget(hints);
        w->setLayout(l);
        addWidget(w);

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
    ptree_->setAlternatingRowColors(true);
//    ptree_->setStyleSheet(
//          "QTreeView::branch{background:palette(base)}"
//          "QTreeWidget::branch::!has-children:selected {background-color: rgb(128, 255, 0);}\n"
//          "QTreeWidget::item:selected { show-decoration-selected: 0; background-color:rgb(128, 255, 0); color:black; border:1px dashed red;}\n"

////          "QTreeWidget::branch::!has-children:selected:alternate {background-color: rgb(0, 0, 0);}\n"

////          "QTreeWidget { show-decoration-selected: 0; selection-color: red; selection-background-color: transparent; }\n"

////          "QTreeWidget::item:selected:!active { background: transparent; }\n"
////          "QTreeWidget::item:selected:active { background: transparent; }\n"
//          );
    
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
    modeltree_(modeltree),
    vt_(nullptr)
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
  if (!vt_)
  {
    vt_=new VisualizerThread(this);
    connect(vt_, &VisualizerThread::finished, this, &ParameterSetDisplay::visualizationUpdateFinished);
    vt_->start();
  }

}


void ParameterSetDisplay::visualizationUpdateFinished()
{
  vt_->deleteLater();
  vt_=nullptr;
}


void VisualizerThread::run()
{
  insight::cad::cache.initRebuild();

  insight::CAD_ParameterSet_Visualizer::UsageTracker ut(psd_->modeltree_);

  for (auto& vz: psd_->visualizers_)
  {
    vz->recreateVisualizationElements(&ut);
  }

  ut.cleanupModelTree();

  insight::cad::cache.finishRebuild();
}

VisualizerThread::VisualizerThread(ParameterSetDisplay* psd)
  : QThread(psd), psd_(psd)
{
}
