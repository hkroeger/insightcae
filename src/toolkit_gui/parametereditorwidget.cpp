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

#include "cadparametersetvisualizer.h"
#include "parametereditorwidget.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkparametersetdisplay.h"

using namespace std;



ParameterEditorWidget::ParameterEditorWidget
(
  QWidget *parent,
  insight::ParameterSetVisualizerPtr viz,
  insight::ParameterSet_ValidatorPtr vali,
  ParameterSetDisplay* display
)
: QSplitter(Qt::Horizontal, parent),
//  defaultParameters_(default_pset),
  model_(/*new IQParameterSetModel(pset, default_pset, this)*/ nullptr),
  vali_(vali),
  viz_(std::dynamic_pointer_cast<insight::CADParameterSetVisualizer>(viz))
{
  insight::CurrentExceptionContext ex("creating parameter set editor");

//  connect(model_, &IQParameterSetModel::parameterSetChanged,
//          this, &ParameterEditorWidget::onParameterSetChanged );

  {
    QWidget *w=new QWidget(this);
    QVBoxLayout *l=new QVBoxLayout;
    w->setLayout(l);

    parameterTreeView_ = new QTreeView(w);
//    parameterTreeView_->setModel(model_);
    parameterTreeView_->setAlternatingRowColors(true);
    parameterTreeView_->expandAll();
    parameterTreeView_->resizeColumnToContents(0);
    parameterTreeView_->resizeColumnToContents(1);
    parameterTreeView_->setContextMenuPolicy(Qt::CustomContextMenu);
    parameterTreeView_->setDragDropMode(QAbstractItemView::DragDrop);
    parameterTreeView_->setDefaultDropAction(Qt::MoveAction);
    l->addWidget(parameterTreeView_);


    QObject::connect(
          parameterTreeView_, &QTreeView::customContextMenuRequested,
          [this](const QPoint& p)
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
    insight::CurrentExceptionContext ex("connecting visualizer");

    // there is a visualization generator available
    connect(
          viz_.get(), &insight::CADParameterSetVisualizer::updateSupplementedInputData,
          this, &ParameterEditorWidget::updateSupplementedInputData
          );

    if (!display)
    {
      insight::CurrentExceptionContext ex("building parameter set displayer");

      // no existing displayer supplied; create one
      viewer_=nullptr;
      {
          insight::CurrentExceptionContext ex("creating CAD viewer");
          viewer_=new CADViewer;
          viewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

          addWidget(viewer_);
      }

      insight::dbg()<<"create model tree"<<std::endl;
      modeltree_ = new QTreeView(this);

      display_ = new ParameterSetDisplay(static_cast<QSplitter*>(this), viewer_, modeltree_);

      // after ParameterSetDisplay constructor!
      modeltree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
      connect(modeltree_->model(), &QAbstractItemModel::dataChanged,
              this, &ParameterEditorWidget::onCADModelDataChanged );
      addWidget(modeltree_);
    }
    else
    {
      // use supplied displayer
      display_=display;
    }
    viz_->setModel(display_->model());
//    viz_->setParameterSetModel(model_);
  }
  else
  {
    display_ = nullptr;
  }

  QObject::connect( parameterTreeView_, &QTreeView::clicked, parameterTreeView_,
           [this](const QModelIndex& index)
           {
             model_->IQParameterSetModel::handleClick(
                         index, inputContents_,
                         display_ ? display_->viewer() : nullptr,
                         viz_.get() );
           }
  );

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



ParameterEditorWidget::ParameterEditorWidget
(
  insight::ParameterSet& pset,
  const insight::ParameterSet& default_pset,
  QWidget *parent,
  insight::ParameterSetVisualizerPtr viz,
  insight::ParameterSet_ValidatorPtr vali,
  ParameterSetDisplay* display
)
: ParameterEditorWidget(parent, viz, vali, display)
{
  resetParameterSet(pset, default_pset);
}


bool ParameterEditorWidget::hasVisualizer() const
{
  return bool(viz_);
}


void ParameterEditorWidget::clearParameterSet()
{
  if (model_)
  {
    parameterTreeView_->setModel(nullptr);
    disconnect(model_, &IQParameterSetModel::parameterSetChanged, this, 0);
    model_->deleteLater();
    model_=nullptr;
  }
}


void ParameterEditorWidget::resetParameterSet(
        insight::ParameterSet& pset,
        const insight::ParameterSet& default_pset
        )
{
  clearParameterSet();

  model_ = new IQParameterSetModel(pset, default_pset, this);
  defaultParameters_=default_pset;

  connect(model_, &IQParameterSetModel::parameterSetChanged,
          this, &ParameterEditorWidget::onParameterSetChanged );

  parameterTreeView_->setModel(model_);
  if (viz_)
  {
    viz_->setParameterSetModel(model_);
  }
}

ParameterEditorWidget::CADViewer *ParameterEditorWidget::viewer() const
{
  return viewer_;
}



void ParameterEditorWidget::onParameterSetChanged()
{
  if (viz_)
  {
    viz_->update(model_->getParameterSet());
  }
  Q_EMIT parameterSetChanged();
}



void ParameterEditorWidget::onCADModelDataChanged
(
      const QModelIndex &topLeft,
      const QModelIndex &bottomRight,
      const QVector<int> &roles )
{
  if (roles.contains(Qt::CheckStateRole))
  {
      disconnect(modeltree_->model(), &QAbstractItemModel::dataChanged,
              this, &ParameterEditorWidget::onCADModelDataChanged );

      auto checkstate = topLeft.data(Qt::CheckStateRole);
      auto indices = modeltree_->selectionModel()->selectedIndexes();
      for (const auto& idx: indices)
      {
          if (idx.column()==topLeft.column())
          {
              modeltree_->model()->setData(
                      idx,
                      checkstate,
                      Qt::CheckStateRole );
          }
      }

      connect(modeltree_->model(), &QAbstractItemModel::dataChanged,
              this, &ParameterEditorWidget::onCADModelDataChanged );
  }
}



