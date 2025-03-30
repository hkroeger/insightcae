#include "iqiscadmodelwindow.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QCloseEvent>

#include "iqcaditemmodel.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqiscadmodelscriptedit.h"

#include "base/translations.h"
#include "iqcadmodel3dviewer/iqvtkcadmodel3dviewersettingsdialog.h"

IQISCADModelWindow::IQISCADModelWindow(QWidget* parent)
: QWidget(parent),
  model_(new IQCADItemModel(insight::cad::ModelPtr(), this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    QSplitter *splitterHoriz=new QSplitter(Qt::Horizontal);
    layout->addWidget(splitterHoriz);

    viewer_=new Model3DViewer(this);
    viewer_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    splitterHoriz->addWidget(viewer_);

    modelEdit_=new IQISCADModelScriptEdit(splitterHoriz);

    auto *widgetVertRight = new QWidget;
    auto *layoutVertRight = new QVBoxLayout;
    widgetVertRight->setLayout(layoutVertRight);
    splitterHoriz->addWidget(widgetVertRight);

    {
        auto *gb=new QGroupBox(_("Controls"));
        // Rebuild control box
        auto *controlWidget = new QWidget;
        auto *shbox = new QHBoxLayout;
        auto *rebuildBtn = new QPushButton(_("Rebuild"), gb);
        auto *rebuildBtnUTC = new QPushButton(_("Rbld to Cursor"), gb);
        shbox->addWidget(rebuildBtn);
        shbox->addWidget(rebuildBtnUTC);
        controlWidget->setLayout(shbox);

        auto *vbox = new QVBoxLayout;
        vbox->addWidget(controlWidget);

        auto *tglHBox = new QHBoxLayout;
        QCheckBox *toggleBgParse=new QCheckBox(_("Do BG parsing"), gb);
        toggleBgParse->setCheckState( Qt::Checked );
        tglHBox->addWidget(toggleBgParse);

        QCheckBox *toggleSkipPostprocActions=new QCheckBox(_("Skip Postproc Actions"), gb);
        toggleSkipPostprocActions->setCheckState( Qt::Checked );
        tglHBox->addWidget(toggleSkipPostprocActions);
        vbox->addLayout(tglHBox);

        gb->setLayout(vbox);

        layoutVertRight->addWidget(gb);

        connect(rebuildBtn, &QPushButton::clicked,
                modelEdit_, &IQISCADModelScriptEdit::rebuildModel);
        connect(rebuildBtnUTC, &QPushButton::clicked,
                modelEdit_, &IQISCADModelScriptEdit::rebuildModelUpToCursor);
        connect(toggleBgParse, &QCheckBox::stateChanged,
                modelEdit_, &IQISCADModelScriptEdit::toggleBgParsing);
        connect(toggleSkipPostprocActions, &QCheckBox::stateChanged,
                modelEdit_, &IQISCADModelScriptEdit::toggleSkipPostprocActions);

    }

    auto splitterVertRight = new QSplitter(Qt::Vertical);
    layoutVertRight->addWidget(splitterVertRight);

    modelEdit_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    splitterVertRight->addWidget(modelEdit_);


    {
        modelTree_ = new QTreeView(viewer_);
        viewer_->addToolBox(modelTree_, _("Model Tree"));
    }

    {
        // notepad editor
        auto *gb=new QGroupBox(_("Notepad"));
        auto *vbox = new QVBoxLayout;
        notepad_=new QTextEdit;

        vbox->addWidget(notepad_);
        QHBoxLayout *ll=new QHBoxLayout;
        QPushButton* copybtn=new QPushButton(_("<< Copy to cursor <<"));
        ll->addWidget(copybtn);
        QPushButton* clearbtn=new QPushButton(_("Clear"));
        ll->addWidget(clearbtn);
        vbox->addLayout(ll);
        gb->setLayout(vbox);
        splitterVertRight->addWidget(gb);

        connect(copybtn, &QPushButton::clicked,
                this, &IQISCADModelWindow::onCopyBtnClicked);
        connect(clearbtn, &QPushButton::clicked,
                notepad_, &QTextEdit::clear);
    }

    splitterVertRight->setStretchFactor(0,4);
    splitterVertRight->setStretchFactor(1,0);

    // splitterHoriz->addWidget(spl2);

    splitterHoriz->setStretchFactor(0,4);
    splitterHoriz->setStretchFactor(1,3);


    modelEdit_->setModel(model_);
    modelTree_->setModel(model_);
    viewer_->setModel(model_);
    viewer_->connectNotepad(notepad_);

    modelTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
                modelTree_, &QTreeView::customContextMenuRequested, model_,
                [this](const QPoint& pos)
    {
        auto idx = modelTree_->indexAt(pos);
        model_->showContextMenu(idx, modelTree_->mapToGlobal(pos), viewer_);
    }
    );

    connect( viewer_, &IQCADModel3DViewer::contextMenuRequested,
             [this](const QModelIndex& index, const QPoint &globalPos)
    {
        model_->showContextMenu(index, globalPos, viewer_);
    }
    );


    connect( model_, &IQCADItemModel::insertIntoNotebook,
             notepad_, &QTextEdit::insertPlainText );
    connect( model_, &IQCADItemModel::highlightInView,
            viewer_, &Model3DViewer::exposeItem );
    connect( model_, &IQCADItemModel::undoHighlightInView,
            viewer_, &Model3DViewer::undoExposeItem );

    connect( modelEdit_, &IQISCADModelScriptEdit::focus,
            viewer_, &Model3DViewer::exposeItem );
    connect( modelEdit_, &IQISCADModelScriptEdit::unfocus,
            viewer_, &Model3DViewer::undoExposeItem );

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

    connect(modelEdit_, &IQISCADModelScriptEdit::updateTitle,
            this, &IQISCADModelWindow::onUpdateTitle);

//    connect(viewer_, &QoccViewWidget::addEvaluationToModel,
//            modeltree_, &QModelTree::onAddEvaluation);

//    connect(viewer_, &QoccViewWidget::insertNotebookText,
    //            this, &IQISCADModelEditor::onInsertNotebookText);
}




IQCADItemModel *IQISCADModelWindow::model()
{
    return model_;
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



void IQISCADModelWindow::viewerSettings()
{
    IQVTKCADModel3DViewerSettingsDialog dlg(viewer_, this);
    dlg.exec();
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
                _("The editor content is not saved.\nSave now?\n"),
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
