#include "iqiscadmodelwindow.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QCloseEvent>

#include "iqcaditemmodel.h"
#include "iqcadmodel3dviewer.h"
#include "iqiscadmodelscriptedit.h"

IQISCADModelWindow::IQISCADModelWindow(QWidget* parent)
: QWidget(parent),
  model_(new IQCADItemModel(insight::cad::ModelPtr(), this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    QSplitter *spl=new QSplitter(Qt::Horizontal);
    layout->addWidget(spl);

    viewer_=new Model3DViewer(this);
    viewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spl->addWidget(viewer_);

    modelEdit_=new IQISCADModelScriptEdit(spl);
    modelEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spl->addWidget(modelEdit_);

    QSplitter* spl2=new QSplitter(Qt::Vertical, spl);
    spl2->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QGroupBox *gb;
    QVBoxLayout *vbox;


    gb=new QGroupBox("Controls");
    gb->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    vbox = new QVBoxLayout;
    QWidget*shw=new QWidget;
    QHBoxLayout *shbox = new QHBoxLayout;
    QPushButton *rebuildBtn=new QPushButton("Rebuild", gb);
    QPushButton *rebuildBtnUTC=new QPushButton("Rbld to Cursor", gb);
    shbox->addWidget(rebuildBtn);
    shbox->addWidget(rebuildBtnUTC);
    shw->setLayout(shbox);
    vbox->addWidget(shw);

    QCheckBox *toggleBgParse=new QCheckBox("Do BG parsing", gb);
    toggleBgParse->setCheckState( Qt::Checked );
    vbox->addWidget(toggleBgParse);

    QCheckBox *toggleSkipPostprocActions=new QCheckBox("Skip Postproc Actions", gb);
    toggleSkipPostprocActions->setCheckState( Qt::Checked );
    vbox->addWidget(toggleSkipPostprocActions);

    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Model Tree");
    gb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    vbox = new QVBoxLayout;
    modelTree_=new QTreeView(gb);
    modelTree_->setMinimumHeight(20);
    vbox->addWidget(modelTree_);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Notepad");
    vbox = new QVBoxLayout;
    notepad_=new QTextEdit;

    vbox->addWidget(notepad_);
    QHBoxLayout *ll=new QHBoxLayout;
    QPushButton* copybtn=new QPushButton("<< Copy to cursor <<");
    ll->addWidget(copybtn);
    QPushButton* clearbtn=new QPushButton("Clear");
    ll->addWidget(clearbtn);
    vbox->addLayout(ll);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    spl2->setStretchFactor(0,0);
    spl2->setStretchFactor(1,4);
    spl2->setStretchFactor(2,0);

    spl->addWidget(spl2);

    spl->setStretchFactor(0,4);
    spl->setStretchFactor(1,3);
    spl->setStretchFactor(2,0);

//    {
//      QList<int> sizes;
//      sizes << 4700 << 3500 << 1700;
//      spl->setSizes(sizes);
//    }

//    {
//      QList<int> sizes;
//      sizes << 2000 << 6000 << 2000;
//      spl2->setSizes(sizes);
//    }


    connect(rebuildBtn, &QPushButton::clicked,
            modelEdit_, &IQISCADModelScriptEdit::rebuildModel);
    connect(rebuildBtnUTC, &QPushButton::clicked,
            modelEdit_, &IQISCADModelScriptEdit::rebuildModelUpToCursor);
    connect(toggleBgParse, &QCheckBox::stateChanged,
            modelEdit_, &IQISCADModelScriptEdit::toggleBgParsing);
    connect(toggleSkipPostprocActions, &QCheckBox::stateChanged,
            modelEdit_, &IQISCADModelScriptEdit::toggleSkipPostprocActions);

    connect(copybtn, &QPushButton::clicked,
            this, &IQISCADModelWindow::onCopyBtnClicked);
    connect(clearbtn, &QPushButton::clicked,
            notepad_, &QTextEdit::clear);

//    modelEdit_->connectModelTree(modelTree_);
    modelEdit_->setModel(model_);
    modelTree_->setModel(model_);
    viewer_->setModel(model_);

    modelTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
                modelTree_, &QTreeView::customContextMenuRequested, model_,
                [this](const QPoint& pos)
                {
                    auto idx = modelTree_->indexAt(pos);
                    model_->showContextMenu(idx, modelTree_->mapToGlobal(pos));
                }
    );
    connect( viewer_, &IQCADModel3DViewer::contextMenuRequested,
             model_, &IQCADItemModel::showContextMenu );


    connect( model_, &IQCADItemModel::insertIntoNotebook,
             notepad_, &QTextEdit::insertPlainText );
    connect( model_, &IQCADItemModel::highlightInView,
             viewer_, &Model3DViewer::highlightItem );
    connect( model_, &IQCADItemModel::undoHighlightInView,
             viewer_, &Model3DViewer::undoHighlightItem );

    connect( modelEdit_, &IQISCADModelScriptEdit::focus,
             viewer_, &Model3DViewer::highlightItem );
    connect( modelEdit_, &IQISCADModelScriptEdit::unfocus,
             viewer_, &Model3DViewer::undoHighlightItem );

#warning reimplement!
//    connect(modeltree_, &QModelTree::showItem,
//            viewer_, &QoccViewWidget::onShow);
//    connect(modeltree_, &QModelTree::hideItem,
//            viewer_, &QoccViewWidget::onHide);
//    connect(modeltree_, &QModelTree::setDisplayMode,
//            viewer_, &QoccViewWidget::onSetDisplayMode);
//    connect(modeltree_, &QModelTree::setColor,
//            viewer_, &QoccViewWidget::onSetColor);
//    connect(modeltree_, &QModelTree::setItemResolution,
//            viewer_, &QoccViewWidget::onSetResolution);

//    connect(modeltree_, &QModelTree::focus,
//            viewer_, &QoccViewWidget::onFocus);
//    connect(modeltree_, &QModelTree::unfocus,
//            viewer_, &QoccViewWidget::onUnfocus);
//    connect(model_, &ISCADModel::focus,
//            viewer_, &QoccViewWidget::onFocus);
//    connect(model_, &ISCADModel::unfocus,
//            viewer_, &QoccViewWidget::onUnfocus);

//    connect(modeltree_, &QModelTree::insertIntoNotebook,
//            this, &IQISCADModelEditor::onInsertNotebookText);

//    connect(model_, &ISCADModel::updateTitle,
//            this, &IQISCADModelEditor::onUpdateTitle);

//    connect(viewer_, &QoccViewWidget::addEvaluationToModel,
//            modeltree_, &QModelTree::onAddEvaluation);

//    connect(viewer_, &QoccViewWidget::insertNotebookText,
//            this, &IQISCADModelEditor::onInsertNotebookText);
}



IQISCADModelScriptEdit* IQISCADModelWindow::modelEdit()
{
    return modelEdit_;
}



IQISCADModelWindow::Model3DViewer* IQISCADModelWindow::viewer()
{
    return viewer_;
}



QTextEdit* IQISCADModelWindow::notepad()
{
    return notepad_;
}



QTreeView* IQISCADModelWindow::modelTree()
{
    return modelTree_;
}



void IQISCADModelWindow::onCopyBtnClicked()
{
  modelEdit_->textCursor().insertText(notepad_->toPlainText());
  notepad_->clear();
}


void IQISCADModelWindow::onInsertNotebookText(const QString& text)
{
  notepad_->insertPlainText(text);
}


void IQISCADModelWindow::onUpdateTitle(const boost::filesystem::path& filepath, bool isUnsaved)
{
  emit updateTabTitle(this, filepath, isUnsaved);
}


void IQISCADModelWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::Yes;

    if (modelEdit_->isUnsaved())
    {
        resBtn =
            QMessageBox::question
            (
                this,
                "ISCAD",
                tr("The editor content is not saved.\nSave now?\n"),
                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                QMessageBox::No
            );

        if (resBtn == QMessageBox::Cancel)
        {
            event->ignore();
            return;
        }
        else
        {
            if (resBtn == QMessageBox::Yes)
            {
                bool saved = modelEdit_->saveModel();
                if (!saved)
                {
                    saved=modelEdit_->saveModelAs();
                }
                if (!saved)
                {
                    event->ignore();
                    return;
                }
            }
        }
    }

    event->accept();
}
