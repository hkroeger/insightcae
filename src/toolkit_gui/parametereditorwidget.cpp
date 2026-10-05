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
#include <QHeaderView>
#include <QTimer>
#include <exception>

#include "base/exception.h"
#include "base/exceptionhandling.h"
#include "boost/algorithm/string/replace.hpp"
#include "cadparametersetvisualizer.h"
#include "iqparametersetvisualizationscheduler.h"
#include "parametereditorwidget.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqvtkparametersetdisplay.h"
#include "qnamespace.h"
#include "qtextensions.h"

#include "base/translations.h"
#include "iqparameters/iqcadsketchparameter.h"


using namespace std;




void ParameterEditorWidget::setup(ParameterSetDisplay* display)
{
    insight::CurrentExceptionContext ex("creating parameter set editor");

    treeFilterModel_ = new IQHierarchicalDataFilterProxyModel(this);

    parameterTreeView_->setAlternatingRowColors(true);
    parameterTreeView_->setContextMenuPolicy(Qt::CustomContextMenu);
    parameterTreeView_->setDragDropMode(QAbstractItemView::DragDrop);
    parameterTreeView_->setDefaultDropAction(Qt::MoveAction);



    QObject::connect(
        parameterTreeView_, &QTreeView::customContextMenuRequested,
        [this](const QPoint& p)
        {
            DBG_SLOT(QTreeView::customContextMenuRequested);

            IQParameterSetModel::contextMenu(
                parameterTreeView_,
                treeFilterModel_->mapToSource(parameterTreeView_->indexAt(p)),
                p,
                viewer_ );
        }
        );

    // e.g. open sketch editor on double click
    IQParameterSetModel::enableActivationOnDoubleClick(
        parameterTreeView_,
        [this]() { return viewer_; } );

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

            viewer_->addToolBox(modeltree, "Model structure");

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
            DBG_SLOT(QTreeView::clicked);

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
                    p->checkEnabledOrDisabled();

                    // connect(this, &ParameterEditorWidget::adaptEditControlsLayout, l,
                    //         [](QSize s){

                    //         });


                    if (vizScheduler_ && viewer_)
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

                    // new controls are shown via queued event; compute height afterwards
                    QTimer::singleShot(0, this, &ParameterEditorWidget::adjustEditPanelHeight);


                    // l->addStretch();
                }
            }
        }
        );

}





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
  createGUIActions_(cgaf)
{

    splitterV_ = new QSplitter(Qt::Vertical);

    {
        QWidget *w=new QWidget(this);
        QVBoxLayout *l=new QVBoxLayout;
        w->setLayout(l);

        inputParametersLabel_=new QLabel("Input Parameters");
        l->addWidget(inputParametersLabel_);
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

        splitterV_->addWidget(w);
    }

    {
        inputContents_=new QWidget(this);
        splitterV_->addWidget(inputContents_);
    }

    addWidget(splitterV_);

    // on resize, keep the height of the edit controls panel
    splitterV_->setStretchFactor(0, 1);
    splitterV_->setStretchFactor(1, 0);

    // splitterMoved is only emitted on user interaction, not by setSizes
    connect(splitterV_, &QSplitter::splitterMoved, this,
            [this](int, int) { editPanelHeightUserAdjusted_=true; } );

    // moving the handle to the 3D view changes the width: re-fit edit panel height
    connect(this, &QSplitter::splitterMoved, this,
            [this](int, int) { adjustEditPanelHeight(); } );

    // double-click on handle restores automatic height
    auto *h = splitterV_->handle(1);
    h->setToolTip("Drag to resize.\nDouble-click to restore automatic height.");
    h->installEventFilter(this);


    setup(display);

    QPalette semiTransparent(QColor(255,0,0,128));
    semiTransparent.setBrush(QPalette::Text, Qt::white);
    semiTransparent.setBrush(QPalette::WindowText, Qt::white);


    if (display_)
    {
        overlayText_ = new IQEphemeralLabel(viewer());
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
        //         [this](const insight::Exception& ex)
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

    // width may have changed: re-fit edit panel height
    adjustEditPanelHeight();

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

    if (splitterV_)
        Q_EMIT adaptEditControlsLayout(splitterV_->size());
}

bool ParameterEditorWidget::eventFilter(QObject* watched, QEvent* event)
{
    // detached panel: resizes are not seen by resizeEvent
    if ( splitterV_
        && watched == splitterV_.data()
        && event->type() == QEvent::Resize )
    {
        adjustEditPanelHeight();
        Q_EMIT adaptEditControlsLayout(splitterV_->size());
    }

    if ( splitterV_
        && watched == splitterV_->handle(1)
        && event->type() == QEvent::MouseButtonDblClick )
    {
        editPanelHeightUserAdjusted_=false;
        adjustEditPanelHeight();
        return true;
    }
    return QSplitter::eventFilter(watched, event);
}

void ParameterEditorWidget::adjustEditPanelHeight()
{
    if (splitterV_ && !editPanelHeightUserAdjusted_)
    {
        auto s=splitterV_->sizes();
        auto sum=s[0]+s[1];

        int h = inputContents_->minimumSizeHint().height();

        if (auto *l = inputContents_->layout())
        {
            l->activate(); // lay out the new controls, so that widths are known
            for (auto *v: inputContents_->findChildren<IQSimpleLatexView*>(
                     QString(), Qt::FindDirectChildrenOnly))
            {
                if (l->indexOf(v)>=0) // skip old controls pending deleteLater
                {
                    // extra height, beyond the minimum, to show the full description
                    h += std::max(0, v->heightForWidth(v->width()) - v->minimumHeight());
                }
            }
        }

        s[1]=std::min(h, sum/2); // at most as high as the tree view
        s[0]=sum-s[1];

        splitterV_->setSizes(s);
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
    viewer_(nullptr)
{
    setup(nullptr);
    viewer_=viewer; // would be nullified in setup
}


ParameterEditorWidget::~ParameterEditorWidget()
{
    // stop visualization computation, before the display (and its CAD model) is destroyed
    delete vizScheduler_;
}



bool ParameterEditorWidget::hasVisualizer() const
{
    return bool(createVisualizer_);
}




QWidget *ParameterEditorWidget::detachParameterPanel()
{
    if (!splitterV_ || !inputParametersLabel_ || isParameterPanelDetached())
        return nullptr;

    splitterV_->setParent(nullptr); // removes it from this splitter
    splitterV_->installEventFilter(this);
    inputParametersLabel_->hide();

    return splitterV_;
}




bool ParameterEditorWidget::isParameterPanelDetached() const
{
    return splitterV_ && (indexOf(splitterV_)<0);
}


void ParameterEditorWidget::setModel(QAbstractItemModel *model)
{
    if (model_)
    {
        disconnectParameterSetChanged(model_, this);
        if (auto *hdm = dynamic_cast<IQHierarchicalDataModel*>(model_))
        {
            QObject::disconnect(hdm, &IQHierarchicalDataModel::bulkUpdateFinished,
                                this, &ParameterEditorWidget::onParameterSetChanged);
        }
        if (!model)
        {
            parameterTreeView_->setModel(nullptr);
            treeFilterModel_->setSourceModel(nullptr);
            if (display_) display_->model()->setAssociatedParameterSetModel(nullptr);
        }
    }

    model_=model;

    connectParameterSetChanged(
        model_,
        this, &ParameterEditorWidget::onParameterSetChanged );

    if (auto *hdm = dynamic_cast<IQHierarchicalDataModel*>(model_))
    {
        connect(hdm, &IQHierarchicalDataModel::bulkUpdateFinished,
                this, &ParameterEditorWidget::onParameterSetChanged);
    }

    treeFilterModel_->setSourceModel(model_);
    parameterTreeView_->setModel(model_ ? treeFilterModel_ : nullptr);
    parameterTreeView_->setIconSize(QSize(32,32));
    parameterTreeView_->setItemDelegate(
        new IQHierarchicalDataGridViewSelectorDelegate);

    if (display_)
    {
        display_->model()->setAssociatedParameterSetModel(model_);
    }

    expandParameterTree();

    parameterTreeView_->header()->setSectionResizeMode(
        QHeaderView::ResizeMode::ResizeToContents);
}




void ParameterEditorWidget::expandParameterTree()
{
    parameterTreeView_->expandToDepth(2);

    // Collapse sketch parameter nodes — their entity children clutter the view
    collapseMatchingNodes(
        parameterTreeView_,
        [](const QModelIndex& idx) {
            auto* iqe = IQHierarchicalDataModel::wrapperFromIndex(idx);
            return iqe && iqe->type() == IQCADSketchParameter::typeName_();
        });
}




void ParameterEditorWidget::setParameterFilter(
    const insight::hierarchicalData::Filter &filter )
{
    treeFilterModel_->resetFilter(filter);
    expandParameterTree();
}




const insight::hierarchicalData::Filter &
ParameterEditorWidget::parameterFilter() const
{
    return treeFilterModel_->filter();
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
        if (!vizScheduler_)
        {
            insight::CurrentExceptionContext ex("creating visualization scheduler");

            vizScheduler_ = new insight::IQParameterSetVisualizationScheduler(
                [this](QObject* parent) -> insight::CADParameterSetVisualizerGenerator*
                {
                    insight::CurrentExceptionContext ex("creating new visualizer");
                    return createVisualizer_(
                        parent,
                        dynamic_cast<IQParameterSetModel*>(model_) );
                },
                display_ ? display_->model() : nullptr,
                100,
                this );

            connect(
                vizScheduler_, &insight::IQParameterSetVisualizationScheduler::updateSupplementedInputData,
                this, &ParameterEditorWidget::updateSupplementedInputData
                );
            connect(
                vizScheduler_, &insight::IQParameterSetVisualizationScheduler::visualizationCalculationFinished, this,
                [this](bool success)
                {
                    DBG_SLOT(insight::CADParameterSetModelVisualizer::visualizationCalculationFinished);

                    if (success && overlayText_) overlayText_->hide();
                } );

            connect(
                vizScheduler_, &insight::IQParameterSetVisualizationScheduler::visualizationComputationError, this,
                [this](std::exception_ptr ex)
                {
                    DBG_SLOT(insight::CADParameterSetModelVisualizer::visualizationComputationError);

                    if (!overlayText_) return;

                    try {
                        std::rethrow_exception(ex);
                    }
                    catch (...)
                    {
                        auto desc=insight::describeCurrentException();
                        overlayText_->setTextFormat(Qt::MarkdownText);
                        overlayText_->setText(QString::fromStdString(
                            std::string(_("The visualization could not be generated completely."))
                            +"\n\n"
                            +_("Reason:")
                            +"\n\n"
                            +boost::replace_all_copy(std::string(*desc), "\n", "\n\n")+"\n\n"
                            +boost::replace_all_copy(desc->context_, "\n", "\n\n")
                            ));
                        overlayText_->show();
                    }
                }
            );
        }

        insight::dbg(insight::DetailedBusiness) << "scheduling visualization update" << std::endl;

        vizScheduler_->requestUpdate();
    }
}



insight::supplementedInputDataBasePtr
ParameterEditorWidget::upToDateSupplementedInputData() const
{
    if (vizScheduler_)
        return vizScheduler_->upToDateSupplementedInputData();
    return nullptr;
}




void ParameterEditorWidget::whenVisualizationIdle(std::function<void()> f)
{
    if (vizScheduler_)
        vizScheduler_->whenIdle(f);
    else
        f();
}




void ParameterEditorWidget::cancelWaitForVisualization()
{
    if (vizScheduler_)
        vizScheduler_->cancelWhenIdle();
}




void ParameterEditorWidget::onParameterSetChanged()
{
    DBG_SLOT(QAbstractItemModel::{dataChanged;rowsInserted;rowsRemoved});

    if (auto *hdm = dynamic_cast<IQHierarchicalDataModel*>(model_))
        if (hdm->isBulkUpdateInProgress())
            return;

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



