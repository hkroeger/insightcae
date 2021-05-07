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

#include <QSplitter>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

#include "parametersetvisualizer.h"
#include "parametereditorwidget.h"

#include "visualizerthread.h"


using namespace std;


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
//  parameters_(pset),
  defaultParameters_(default_pset),
  model_(new IQParameterSetModel(pset, default_pset, this)),
  vali_(vali),
  viz_(std::dynamic_pointer_cast<insight::CAD_ParameterSet_Visualizer>(viz))
{

  connect(model_, &IQParameterSetModel::parameterSetChanged,
          this, &ParameterEditorWidget::onParameterSetChanged );

  {
    QWidget *w=new QWidget(this);
    QVBoxLayout *l=new QVBoxLayout(w);
    w->setLayout(l);

    parameterTreeView_ = new QTreeView(w);
    parameterTreeView_->setModel(model_);
    parameterTreeView_->setAlternatingRowColors(true);
    parameterTreeView_->expandAll();
    parameterTreeView_->resizeColumnToContents(0);
    parameterTreeView_->resizeColumnToContents(1);
    parameterTreeView_->setContextMenuPolicy(Qt::CustomContextMenu);
    l->addWidget(parameterTreeView_);

    QObject::connect( parameterTreeView_, &QTreeView::clicked,
             [=](const QModelIndex& index)
             {
               model_->IQParameterSetModel::handleClick(index, inputContents_);
             }
    );

    QObject::connect(
          parameterTreeView_, &QTreeView::customContextMenuRequested,
          [=](const QPoint& p)
    {
      model_->contextMenu( parameterTreeView_,
                           parameterTreeView_->indexAt(p),
                           p );
    }
    );



    QLabel *hints=new QLabel(w);
    hints->setStyleSheet("font: 8pt;");
    hints->setText(
          "Please edit the parameters in the list above.\n"
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
      QWidget *w=new QWidget(this);
      QVBoxLayout *l=new QVBoxLayout(w);
      QoccViewWidget *viewer=new QoccViewWidget(w/*, context->getContext()*/);
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

      display_=new ParameterSetDisplay(static_cast<QSplitter*>(this), viewer, modeltree);
      display_->connectVisualizer(viz_);
    }
    else
    {
      // use supplied displayer
      display_=display;
    }
    connect(this, &ParameterEditorWidget::parameterSetChanged,
            viz_.get(), &insight::CAD_ParameterSet_Visualizer::visualizeScheduledParameters);
  }
  else
  {
    display_ = nullptr;
  }

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

}


bool ParameterEditorWidget::hasVisualizer() const
{
  return bool(viz_);
}


void ParameterEditorWidget::onParameterSetChanged()
{
  if (viz_)
  {
    viz_->update(model_->getParameterSet());
  }
  Q_EMIT parameterSetChanged();
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
{}

void ParameterSetDisplay::connectVisualizer(std::shared_ptr<insight::CAD_ParameterSet_Visualizer> viz)
{
  if (viz)
  {
    modeltree_->connectModel(viz.get());
  }
}

void ParameterSetDisplay::disconnectVisualizer(std::shared_ptr<insight::CAD_ParameterSet_Visualizer> viz)
{
  if (viz)
  {
    modeltree_->disconnectModel(viz.get());
  }
}




