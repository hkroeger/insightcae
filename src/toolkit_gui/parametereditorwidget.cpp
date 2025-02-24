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

#include "base/exception.h"
#include "boost/algorithm/string/replace.hpp"
#include "cadparametersetvisualizer.h"
#include "parametereditorwidget.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkparametersetdisplay.h"
#include "qnamespace.h"

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

    if (hasVisualizer())
    {
        insight::CurrentExceptionContext ex("setting up 3D viewer");

        if (!display)
        {
            insight::CurrentExceptionContext ex("building parameter set displayer");

            auto viewer=new CADViewer;
            viewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

            addWidget(viewer);

            insight::dbg()<<"create model tree"<<std::endl;
            auto modeltree = new QTreeView(this);

            display_ = new ParameterSetDisplay(static_cast<QSplitter*>(this), viewer, modeltree);
            viewer_ = display_->viewer();

            viewer_->commonToolBox()->addItem(modeltree, "Model structure");

            // after ParameterSetDisplay constructor!
            modeltree->setSelectionMode(QAbstractItemView::ExtendedSelection);
            connect(modeltree, &QTreeView::clicked,
                    this, &ParameterEditorWidget::onItemClicked );
        }
        else
        {
            // use supplied displayer
            display_=display;
            viewer_=display_->viewer();
        }
    }
    else
    {
        display_ = nullptr;
        viewer_=nullptr;
    }

    QObject::connect(
        parameterTreeView_, &QTreeView::clicked, parameterTreeView_,
        [this](const QModelIndex& index)
        {
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
                        viewer_ );


                    if (viz_ && viewer_)
                    {
                        if (createGUIActions_)
                        {
                            auto cmdl=new QHBoxLayout;
                            auto acts = createGUIActions_(
                                p->get()->path(),
                                inputContents_,
                                viewer_,
                                dynamic_cast<IQParameterSetModel*>(model_)
                            );
                            for (auto& act: acts)
                            {
                                auto btn = new QPushButton(act.icon, act.label);
                                connect(btn, &QPushButton::clicked, btn,
                                        act.action);
                                cmdl->addWidget(btn);
                            }
                            l->addLayout(cmdl);
                        }
                    }

                    l->addStretch();
                }
            }
        }
        );

}



void ParameterEditorWidget::showEvent(QShowEvent *event)
{
    QSplitter::showEvent(event);

    if (!firstShowOccurred_)
    {
        firstShowOccurred_=true;

        parameterTreeView_->expandToDepth(2);
        parameterTreeView_->resizeColumnToContents(0);
        parameterTreeView_->resizeColumnToContents(1);
    }
}


class MLabel : public QLabel
{
public:
    MLabel(QWidget *parent) : QLabel(parent) {};

    void mousePressEvent(QMouseEvent *event) override
    {
        QLabel::mousePressEvent(event);
        hide();
    }
};


ParameterEditorWidget::ParameterEditorWidget
(
  QWidget *parent,
  insight::PECADParameterSetVisualizerBuilder psvb,
  insight::CADParameterSetModelVisualizer::CreateGUIActionsFunctions::Function cgaf,
  insight::ParameterSet_ValidatorPtr vali,
  ParameterSetDisplay* display
)
: QSplitter(Qt::Horizontal, parent),
  model_(nullptr),
  vali_(vali),
    createVisualizer_(psvb),
    createGUIActions_(cgaf),
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



    setup(display);

    QPalette semiTransparent(QColor(255,0,0,128));
    semiTransparent.setBrush(QPalette::Text, Qt::white);
    semiTransparent.setBrush(QPalette::WindowText, Qt::white);


    if (display_)
    {
        overlayText_ = new MLabel(viewer());
        overlayText_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        overlayText_->setAutoFillBackground(true);
        overlayText_->setPalette(semiTransparent);
        overlayText_->setMargin(10);
        QFont f = font();
        f.setPixelSize(QFontInfo(f).pixelSize()*1.5);
        overlayText_->setFont(f);
        overlayText_->hide();


        // connect(
        //     viz_.get(), &insight::CADParameterSetModelVisualizer::visualizationCalculationFinished, viz_.get(),
        //     [this](bool success)
        //     { if (success) overlayText_->hide(); } );

        // if (viz_)
        // {
        //     connect(
        //         viz_.get(), &insight::CADParameterSetModelVisualizer::visualizationComputationError, viz_.get(),
        //         [this](insight::Exception ex)
        //         {
        //             overlayText_->setTextFormat(Qt::MarkdownText);
        //             std::ostringstream msg;
        //             msg
        //             << "The visualization could not be generated.\n\n"
        //                "Reason:\n\n"
        //                "**"+ex.description()+"**\n\n"
        //                 <<boost::replace_all_copy(ex.context(), "\n", "\n\n")
        //                 ;

        //             // if (auto *cae = dynamic_cast<const insight::CADException*>(&ex))
        //             // {
        //             //     auto gm = cae->contextGeometry();
        //             //     for (auto g: gm)
        //             //     {
        //             //         std::string fn("errorContext_"+g.first+".brep");
        //             //         BRepTools::Write(
        //             //             *g.second,
        //             //             fn.c_str()
        //             //         );
        //             //         msg<<"saved context to "<<fn<<"\n";
        //             //     }
        //             // }

        //             overlayText_->setText(QString::fromStdString(msg.str()));
        //             overlayText_->show();
        //         }
        //     );
        // }
    }
}

void ParameterEditorWidget::resizeEvent(QResizeEvent*e)
{
    QSplitter::resizeEvent(e);

    if (display_)
    {
        auto w = viewer()->centralWidget()->width();
        auto h = viewer()->centralWidget()->height();

        int m = w / 10;
        overlayText_->setGeometry(
            m, m,
            w - 2 * m,
            h - 2 * m );
    }
}

ParameterEditorWidget::ParameterEditorWidget
(
    QWidget *parent,
    QTreeView* parameterTreeView,
    QWidget* contentEditorFrame,
    IQCADModel3DViewer* viewer
)
    : QSplitter(Qt::Horizontal, parent),
    model_(nullptr),
    vali_(nullptr),
    parameterTreeView_(parameterTreeView),
    inputContents_(contentEditorFrame),
    firstShowOccurred_(false),
    viewer_(nullptr)
{
    setup(nullptr);
    viewer_=viewer; // would be nullified in setup
}



bool ParameterEditorWidget::hasVisualizer() const
{
    return bool(createVisualizer_);
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
    parameterTreeView_->setItemDelegate(
        new IQParameterGridViewSelectorDelegate);

    if (display_)
    {
        display_->model()->setAssociatedParameterSetModel(model_);
    }

    parameterTreeView_->expandToDepth(2);
    parameterTreeView_->resizeColumnToContents(0);
    parameterTreeView_->resizeColumnToContents(1);
}

bool ParameterEditorWidget::hasViewer() const
{
    return viewer_!=nullptr;
}


ParameterEditorWidget::CADViewer *ParameterEditorWidget::viewer() const
{
    auto *v = dynamic_cast<CADViewer*>(viewer_);
    insight::assertion(
        bool(v), "unexpected viewer type" );
    return v;
}



void ParameterEditorWidget::rebuildVisualization()
{
    if (hasVisualizer())
    {
        if (viz_ && !viz_->isFinished())
        {
            viz_->stopVisualizationComputation();
            viz_->deleteLater();
        }


        viz_ = createVisualizer_(
                this,
                dynamic_cast<IQParameterSetModel*>(model_)
            );

        connect(
            viz_, &insight::CADParameterSetModelVisualizer::updateSupplementedInputData,
            this, &ParameterEditorWidget::updateSupplementedInputData
            );
        connect(
            viz_, &insight::CADParameterSetModelVisualizer::visualizationCalculationFinished, viz_,
            [this](bool success)
            { if (success) overlayText_->hide(); } );

        connect(
            viz_, &insight::CADParameterSetModelVisualizer::visualizationComputationError, viz_,
            [this](insight::Exception ex)
            {
                overlayText_->setTextFormat(Qt::MarkdownText);
                overlayText_->setText(QString::fromStdString(
                    "The visualization could not be generated.\n\n"
                    "Reason:\n\n"
                    "**"+ex.description()+"**\n\n"+
                    boost::replace_all_copy(ex.context(), "\n", "\n\n")
                    ));
                overlayText_->show();
            }
        );

        viz_->launch(display_->model());
    }
}



void ParameterEditorWidget::onParameterSetChanged()
{
    rebuildVisualization();
    Q_EMIT parameterSetChanged();
}



void ParameterEditorWidget::onItemClicked
(
      const QModelIndex &item )
{

#warning reimplement multi show/hide
    // if (display_)
    // {
    //     auto modeltree=display_->modeltree();

    //     if (roles.contains(Qt::CheckStateRole)
    //         && topLeft.column()<=IQCADItemModel::visibilityCol
    //         && bottomRight.column()>=IQCADItemModel::visibilityCol)
    //     {

    //         // disconnect(modeltree->model(), &QAbstractItemModel::dataChanged,
    //         //            this, &ParameterEditorWidget::onCADModelDataChanged );

    //         // extend visibility toggling action to all selected items
    //         auto checkstate = topLeft.data(Qt::CheckStateRole);
    //         auto indices = modeltree->selectionModel()->selectedIndexes();
    //         std::set<QModelIndex> toExtTo;
    //         for (const auto& idx: indices) // through all selected
    //         {
    //             if (idx.column()==IQCADItemModel::visibilityCol) // if is the right col
    //             {
    //                 if (idx.parent()==topLeft.parent()) // if same parent
    //                 {
    //                     if (! (
    //                         (topLeft.row()<=idx.row()) &&
    //                         (idx.row()<=bottomRight.row()))) // if not already modified
    //                     {
    //                             toExtTo.insert(idx);
    //                     }
    //                 }
    //             }
    //         }

    //         for (auto &idx: toExtTo)
    //         {
    //             modeltree->model()->setData(
    //                 idx,
    //                 checkstate,
    //                 Qt::CheckStateRole );
    //         }

    //         // connect(modeltree->model(), &QAbstractItemModel::dataChanged,
    //         //         this, &ParameterEditorWidget::onCADModelDataChanged );
    //     }
    // }
}



