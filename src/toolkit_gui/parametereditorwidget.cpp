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
#include <QPushButton>

#include "cadparametersetvisualizer.h"
#include "parametereditorwidget.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkparametersetdisplay.h"

using namespace std;




void ParameterEditorWidget::setup(ParameterSetDisplay* display)
{
    insight::CurrentExceptionContext ex("creating parameter set editor");

    parameterTreeView_->setAlternatingRowColors(true);
    parameterTreeView_->setContextMenuPolicy(Qt::CustomContextMenu);
    parameterTreeView_->setDragDropMode(QAbstractItemView::DragDrop);
    parameterTreeView_->setDefaultDropAction(Qt::MoveAction);



    QObject::connect(
        parameterTreeView_, &QTreeView::customContextMenuRequested,
        [this](const QPoint& p)
        {
            IQParameterSetModel::contextMenu(
                parameterTreeView_,
                parameterTreeView_->indexAt(p),
                p );
        }
    );


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

            display_ = new ParameterSetDisplay(static_cast<QSplitter*>(this), viewer_, modeltree_);

            // after ParameterSetDisplay constructor!
            modeltree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
            connect(modeltree_->model(), &QAbstractItemModel::dataChanged,
                    this, &ParameterEditorWidget::onCADModelDataChanged );
        }
        else
        {
            // use supplied displayer
            display_=display;
        }
        viz_->setModel(display_->model());
    }
    else
    {
        display_ = nullptr;
    }

    QObject::connect( parameterTreeView_, &QTreeView::clicked, parameterTreeView_,
                     [this](const QModelIndex& index)
                     {
//                         model_->IQParameterSetModel::handleClick(
//                             index, inputContents_,
//                             display_ ? display_->viewer() : nullptr,
//                             viz_.get() );
                            if (index.isValid())
                            {
                                if (auto* p=IQParameterSetModel::parameterFromIndex(index))
                                {
                                    // remove existing controls first
                                    {
                                        // clear contents of widget with edit controls
                                        QList<QWidget*> widgets = inputContents_->findChildren<QWidget*>();
                                        foreach(QWidget* widget, widgets)
                                        {
                                            widget->deleteLater();
                                        }
                                        // remove old layout
                                        if (inputContents_->layout())
                                        {
                                            delete inputContents_->layout();
                                        }
                                    }

                                    // create new controls
                                    auto l = p->populateEditControls(
                                        inputContents_,
                                        display_ ? display_->viewer() : nullptr );


                                    if (display_)
                                    {
                                        auto cmdl=new QHBoxLayout;
                                        auto acts = viz_->createGUIActions(
                                            p->path().toStdString(),
                                            inputContents_,
                                            display_->viewer());
                                        for (auto& act: acts)
                                        {
                                            auto btn = new QPushButton(act.icon, act.label);
                                            connect(btn, &QPushButton::clicked, btn,
                                                    act.action);
                                            cmdl->addWidget(btn);
                                        }
                                        l->addLayout(cmdl);
                                    }

                                    l->addStretch();
                                }
                            }
                     }
                     );

    if (modeltree_)
    {
        if (display_)
            display_->viewer()->commonToolBox()->addItem(modeltree_, "Model structure");
        else
            addWidget(modeltree_);
    }

}



void ParameterEditorWidget::showEvent(QShowEvent *event)
{
    QSplitter::showEvent(event);

    if (!firstShowOccurred_)
    {
        firstShowOccurred_=true;

        parameterTreeView_->expandAll();
        parameterTreeView_->resizeColumnToContents(0);
        parameterTreeView_->resizeColumnToContents(1);
    }
}




ParameterEditorWidget::ParameterEditorWidget
(
  QWidget *parent,
  insight::ParameterSetVisualizerPtr viz,
  insight::ParameterSet_ValidatorPtr vali,
  ParameterSetDisplay* display
)
: QSplitter(Qt::Horizontal, parent),
  model_(nullptr),
  vali_(vali),
  viz_(std::dynamic_pointer_cast<insight::CADParameterSetVisualizer>(viz)),
  firstShowOccurred_(false)
{

    QWidget *w=new QWidget(this);
    QVBoxLayout *l=new QVBoxLayout;
    w->setLayout(l);

    parameterTreeView_ = new QTreeView(w);
    l->addWidget(parameterTreeView_);

    QLabel *hints=new QLabel(w);
    hints->setStyleSheet("font: 8pt;");
    hints->setText(
        "Please edit the parameters in the list above.\n"
        "Yellow background: need to revised for each case.\n"
        "Light gray: can usually be left on default values."
        );
    l->addWidget(hints);
    // addWidget(w);

    auto spv = new QSplitter(Qt::Vertical);
    spv->addWidget(w);
    inputContents_=new QWidget(this);
    //addWidget(inputContents_);
    spv->addWidget(inputContents_);
    addWidget(spv);


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
    // addWidget(modeltree_);

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

    setup(display);
}



ParameterEditorWidget::ParameterEditorWidget
(
    QWidget *parent,
    QTreeView* parameterTreeView,
    QWidget* contentEditorFrame
)
    : QSplitter(Qt::Horizontal, parent),
    model_(nullptr),
    vali_(nullptr),
    viz_(nullptr),
    parameterTreeView_(parameterTreeView),
    inputContents_(contentEditorFrame),
    modeltree_(nullptr),
    firstShowOccurred_(false)
{
    setup(nullptr);
}


//ParameterEditorWidget::ParameterEditorWidget
//(
//  insight::ParameterSet& pset,
//  const insight::ParameterSet& default_pset,
//  QWidget *parent,
//  insight::ParameterSetVisualizerPtr viz,
//  insight::ParameterSet_ValidatorPtr vali,
//  ParameterSetDisplay* display
//)
//: ParameterEditorWidget(parent, viz, vali, display)
//{
//  resetParameterSet(pset, default_pset);
//}


bool ParameterEditorWidget::hasVisualizer() const
{
    return bool(viz_);
}


void ParameterEditorWidget::setModel(QAbstractItemModel *model)
{
    if (!model && model_)
    {
        disconnectParameterSetChanged(model_, this);
        parameterTreeView_->setModel(nullptr);
        if (display_) display_->model()->setAssociatedParameterSetModel(nullptr);
    }

    model_=model;

    connectParameterSetChanged(
        model_,
        this, &ParameterEditorWidget::onParameterSetChanged );

    parameterTreeView_->setModel(model_);
    if (viz_)
    {
        if (auto *psm = dynamic_cast<IQParameterSetModel*>(model_))
            viz_->setParameterSetModel(psm);
    }
    if (display_) display_->model()->setAssociatedParameterSetModel(model_);

    parameterTreeView_->expandAll();
    parameterTreeView_->resizeColumnToContents(0);
    parameterTreeView_->resizeColumnToContents(1);
}


//void ParameterEditorWidget::clearParameterSet()
//{
//  if (model_)
//  {
//    parameterTreeView_->setModel(nullptr);
////    disconnect(model_, &IQParameterSetModel::parameterSetChanged, this, 0);
//    disconnectParameterSetChanged(model_, this);
//    model_->deleteLater();
//    model_=nullptr;
//    display_->model()->setAssociatedParameterSetModel(nullptr);
//  }
//}


//void ParameterEditorWidget::resetParameterSet(
//        insight::ParameterSet& pset,
//        const insight::ParameterSet& default_pset
//        )
//{
//  clearParameterSet();

//  model_ = new IQParameterSetModel(pset, default_pset, this);
//  defaultParameters_=default_pset;

////  connect(model_, &IQParameterSetModel::parameterSetChanged,
////          this, &ParameterEditorWidget::onParameterSetChanged );
//  connectParameterSetChanged(
//      model_,
//      this, &ParameterEditorWidget::onParameterSetChanged );

//  parameterTreeView_->setModel(model_);
//  if (viz_)
//  {
//    if (auto *psm = dynamic_cast<IQParameterSetModel*>(model_))
//        viz_->setParameterSetModel(psm);
//  }
//  display_->model()->setAssociatedParameterSetModel(model_);
//}

ParameterEditorWidget::CADViewer *ParameterEditorWidget::viewer() const
{
  return viewer_;
}



void ParameterEditorWidget::onParameterSetChanged()
{
  if (viz_)
  {
    viz_->update(getParameterSet(model_));
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



