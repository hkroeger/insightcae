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
 */

#include "iscadmodel.h"

#include "qmodeltree.h"
#include "qmodelstepitem.h"
#include "qvariableitem.h"
#include "qdatumitem.h"
#include "qevaluationitem.h"
#include "iscadsyntaxhighlighter.h"
#include "occtools.h"

#include "base/boost_include.h"

#include <QSignalMapper>

#include "modelfeature.h"
#include "modelcomponentselectordlg.h"
#include "insertfeaturedlg.h"

#include "datum.h"






ISCADModel::ISCADModel(QWidget* parent, bool dobgparsing)
: QTextEdit(parent),
  unsaved_(false),
  doBgParsing_(dobgparsing),
  bgparsethread_(),
  skipPostprocActions_(true)
{
    setFontFamily("Monospace");
    setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect
        (
          this, SIGNAL(selectionChanged()),
          this, SLOT(onEditorSelectionChanged())
        );

    connect
        (
          this, SIGNAL(customContextMenuRequested(const QPoint&)),
          this, SLOT(showEditorContextMenu(const QPoint&))
        );

    highlighter_=new ISCADSyntaxHighlighter(document());

    connect(&bgparsethread_, SIGNAL(finished()),
            this, SLOT(onBgParseFinished()));
    connect(&bgparsethread_, SIGNAL(statusMessage(const QString&)),
            this, SIGNAL(displayStatusMessage(const QString&)));
    connect(&bgparsethread_, SIGNAL(statusProgress(int, int)),
            this, SIGNAL(statusProgress(int, int)));
    connect(&bgparsethread_, SIGNAL(scriptError(long,const QString&, int)),
            this, SLOT(onScriptError(long,const QString&, int)));
    bgparseTimer_=new QTimer(this);
    connect(bgparseTimer_, SIGNAL(timeout()), this, SLOT(doBgParse()));
    restartBgParseTimer();

    connect(document(), SIGNAL(contentsChange(int,int,int)),
            this, SLOT(restartBgParseTimer(int,int,int)));
    connect(document(), SIGNAL(contentsChange(int,int,int)),
            this, SLOT(setUnsavedState(int,int,int)));

}


ISCADModel::~ISCADModel()
{
//     bgparsethread_.stop();
    bgparsethread_.wait();
}



bool ISCADModel::saveModel()
{
    if (filename_!="")
    {
        std::ofstream out(filename_.c_str());
        out << toPlainText().toStdString();
        out.close();
        unsetUnsavedState();
        return true;
    }
    else
    {
        return false;
    }
}




bool ISCADModel::saveModelAs()
{
    QString fn=QFileDialog::getSaveFileName(this, "Select location", "", "ISCAD Model Files (*.iscad)");
    if (fn!="")
    {
        setFilename(qPrintable(fn));
        saveModel();
        return true;
    }
    else
    {
        return false;
    }
}




void ISCADModel::clearDerivedData()
{
  clear();
  emit clearData();
}


void ISCADModel::loadFile(const boost::filesystem::path& file)
{
    if (!boost::filesystem::exists(file))
    {
        QMessageBox::critical(this, "File error", ("The file "+file.string()+" does not exists!").c_str());
        return;
    }

    clearDerivedData();

    setFilename(file);
    std::ifstream in(file.c_str());

    std::string contents_raw;
    in.seekg(0, std::ios::end);
    contents_raw.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents_raw[0], contents_raw.size());

    setScript(contents_raw);
}


void ISCADModel::setScript(const std::string& contents)
{
    clearDerivedData();

    disconnect
        (
          document(), SIGNAL(contentsChange(int,int,int)),
          this, SLOT(setUnsavedState(int,int,int))
        );

    setPlainText(contents.c_str());

    connect
        (
          document(), SIGNAL(contentsChange(int,int,int)),
          this, SLOT(setUnsavedState(int,int,int))
        );
}


void ISCADModel::onEditorSelectionChanged()
{
    QTextDocument *doc = document();
    QString word=textCursor().selectedText();
    if (!(word.contains('|')||word.contains('*')))
    {
        highlighter_->setHighlightWord(word);
        highlighter_->rehighlight();
    }
}




void ISCADModel::jumpTo(const QString& name)
{
    highlighter_->setHighlightWord(name);
    highlighter_->rehighlight();

    QRegExp expression("\\b("+name+")\\b");
    int i=expression.indexIn(toPlainText());

//   qDebug()<<"jumping "<<name<<" at i="<<i<<endl;

    if (i>=0)
    {
        QTextCursor tmpCursor = textCursor();
        tmpCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1 );
        tmpCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, i );
        setTextCursor(tmpCursor);
    }
}




void ISCADModel::restartBgParseTimer(int,int,int)
{
    bgparseTimer_->setSingleShot(true);
    bgparseTimer_->start(bgparseInterval);
}



void ISCADModel::doBgParse()
{
    if (doBgParsing_)
    {
        if (!bgparsethread_.isRunning())
        {
            emit displayStatusMessage("Background model parsing in progress...");
            bgparsethread_.launch( toPlainText().toStdString() );
        }
    }
}

void ISCADModel::onBgParseFinished()
{
    if (bgparsethread_.model_)
    {
        cur_model_ = bgparsethread_.model_;
        syn_elem_dir_ = bgparsethread_.syn_elem_dir_;

        if (bgparsethread_.action()==BGParsingThread::Rebuild)
        {
        }

        emit modelUpdated();
    }
    emit statusProgress(1, 1);
}


//void ISCADModel::allShaded()
//{
//    modeltree_->setUniformDisplayMode(AIS_Shaded);
//}

//void ISCADModel::allWireframe()
//{
//    modeltree_->setUniformDisplayMode(AIS_WireFrame);
//}




void ISCADModel::populateClipPlaneMenu(QMenu* clipplanemenu, QoccViewWidget* viewer)
{
    clipplanemenu->clear();
    
    if (cur_model_)
    {
        int added=0;
        insight::cad::Model::DatumTableContents datums = cur_model_->datums();
        BOOST_FOREACH(insight::cad::Model::DatumTableContents::value_type const& v, datums)
        {
            insight::cad::DatumPtr d = v.second;
            if (d->providesPlanarReference())
            {
                QAction *act = new QAction( v.first.c_str(), this );
                
                QSignalMapper *signalMapper = new QSignalMapper(this);
                signalMapper->setMapping( act, reinterpret_cast<QObject*>(d.get()) );
                connect
                    (
                      signalMapper, SIGNAL(mapped(QObject*)),
                      viewer, SLOT(onSetClipPlane(QObject*))
                    );
                connect
                    (
                      act, SIGNAL(triggered()),
                      signalMapper, SLOT(map())
                    );
                clipplanemenu->addAction(act);
                
                added++;
            }
        }
        
        if (added)
        {
            clipplanemenu->setEnabled(true);
        }
        else
        {
            clipplanemenu->setDisabled(true);
        }
    }
}



void ISCADModel::connectModelTree(QModelTree* mt) const
{
  connect(&bgparsethread_, SIGNAL(createdVariable(const QString&,insight::cad::ScalarPtr)),
          mt, SLOT(onAddScalar(const QString&,insight::cad::ScalarPtr)));
  connect(&bgparsethread_, SIGNAL(createdVariable(const QString&,insight::cad::VectorPtr)),
          mt, SLOT(onAddVector(const QString&,insight::cad::VectorPtr)));
  connect(&bgparsethread_, SIGNAL(createdFeature(const QString&,insight::cad::FeaturePtr,bool)),
          mt, SLOT(onAddFeature(const QString&,insight::cad::FeaturePtr,bool)));
  connect(&bgparsethread_, SIGNAL(createdDatum(QString,insight::cad::DatumPtr)),
          mt, SLOT(onAddDatum(const QString&,insight::cad::DatumPtr)));
  connect(&bgparsethread_, SIGNAL(createdEvaluation(const QString&,insight::cad::PostprocActionPtr)),
          mt, SLOT(onAddEvaluation(const QString&,insight::cad::PostprocActionPtr)));

  connect(&bgparsethread_, SIGNAL(removedScalar(const QString&)),
          mt, SLOT(onRemoveScalar(const QString&)));
  connect(&bgparsethread_, SIGNAL(removedVector(const QString&)),
          mt, SLOT(onRemoveVector(const QString&)));
  connect(&bgparsethread_, SIGNAL(removedFeature(const QString&)),
          mt, SLOT(onRemoveFeature(const QString&)));
  connect(&bgparsethread_, SIGNAL(removedDatum(const QString&)),
          mt, SLOT(onRemoveDatum(const QString&)));
  connect(&bgparsethread_, SIGNAL(removedEvaluation(const QString&)),
          mt, SLOT(onRemoveEvaluation(const QString&)));

  connect(mt, SIGNAL(jumpTo(QString)),
          this, SLOT(jumpTo(QString)));
  connect(mt, SIGNAL(insertParserStatementAtCursor(QString)),
          this, SLOT(insertTextAtCursor(QString)));

}


void ISCADModel::onGraphicalSelectionChanged(QoccViewWidget* aView)
{
}


void ISCADModel::editSketch(QObject* sk_ptr)
{
    insight::cad::Sketch* sk = reinterpret_cast<insight::cad::Sketch*>(sk_ptr);
    sk->executeEditor();
}


void ISCADModel::editModel(QObject* sk_ptr)
{
    insight::cad::ModelFeature* sk = reinterpret_cast<insight::cad::ModelFeature*>(sk_ptr);
    emit openModel(sk->modelfile());
}




void ISCADModel::toggleBgParsing(int state)
{
    if (state==Qt::Checked)
    {
        doBgParsing_=true;
    }
    else if (state==Qt::Unchecked)
    {
        doBgParsing_=false;
    }
}


void ISCADModel::toggleSkipPostprocActions(int state)
{
    if (state==Qt::Checked)
    {
        skipPostprocActions_=true;
    }
    else if (state==Qt::Unchecked)
    {
        skipPostprocActions_=false;
    }
}


void ISCADModel::insertSectionCommentAtCursor()
{
    textCursor().insertText("\n\n############################################################\n#####  ");
}

void ISCADModel::insertFeatureAtCursor()
{
    InsertFeatureDlg *dlg=new InsertFeatureDlg(this);
    if ( dlg->exec() == QDialog::Accepted )
    {
        textCursor().insertText(dlg->insert_string_);
    }
}


void ISCADModel::insertComponentNameAtCursor()
{
    ModelComponentSelectorDlg* dlg=new ModelComponentSelectorDlg(cur_model_, this);
    if ( dlg->exec() == QDialog::Accepted )
    {
        std::string id = dlg->selected();
        textCursor().insertText(id.c_str());
    }
}





void ISCADModel::rebuildModel(bool upToCursor)
{
    if (!bgparsethread_.isRunning())
    {
        std::string script_content = toPlainText().toStdString();
        if (upToCursor)
        {
            QTextCursor c = textCursor();
            script_content = script_content.substr(0, c.position());
        }

        bgparsethread_.launch
            (
              script_content,
              skipPostprocActions_ ? BGParsingThread::Rebuild : BGParsingThread::Post
            );

    }
    else
    {
        emit displayStatusMessage("Background model parsing in progress, rebuild is currently disabled!...");
    }
}


void ISCADModel::rebuildModelUpToCursor()
{
    rebuildModel(true);
}

void ISCADModel::clearCache()
{
    insight::cad::cache.clear();
}




void ISCADModel::showEditorContextMenu(const QPoint& pt)
{
    QMenu * menu = createStandardContextMenu();

    if (syn_elem_dir_)
    {
        QTextCursor cursor = cursorForPosition(pt);

        std::size_t po=/*editor_->textCursor()*/cursor.position();
        insight::cad::FeaturePtr fp=syn_elem_dir_->findElement(po);
        if (fp)
        {
            QSignalMapper *signalMapper = new QSignalMapper(this);
            QAction *act=NULL;

            insight::cad::Feature* fpp=fp.get();
            if (insight::cad::Sketch* sk=dynamic_cast<insight::cad::Sketch*>(fpp))
            {
                act=new QAction("Edit Sketch...", this);
                signalMapper->setMapping(act, reinterpret_cast<QObject*>(sk));
                connect(signalMapper, SIGNAL(mapped(QObject*)),
                        this, SLOT(editSketch(QObject*)));
            }
            else if (insight::cad::ModelFeature* mo=dynamic_cast<insight::cad::ModelFeature*>(fpp))
            {
                act=new QAction("Edit Model...", this);
                signalMapper->setMapping(act, reinterpret_cast<QObject*>(mo));
                connect(signalMapper, SIGNAL(mapped(QObject*)),
                        this, SLOT(editModel(QObject*)));
            }
            else
            {
//                 std::cout<<"NO"<<std::endl;
            }

            if (act)
            {
                connect(act, SIGNAL(triggered()),
                        signalMapper, SLOT(map()));
                menu->addSeparator();
                menu->addAction(act);
            }
        }
    }

    menu->exec(mapToGlobal(pt));
}


void ISCADModel::insertTextAtCursor(const QString& snippet)
{
    textCursor().insertText(snippet);
}


void ISCADModel::setUnsavedState(int, int rem, int ad)
{
    if ((rem>0) || (ad>0))
    {
        if (!unsaved_)
        {
            unsaved_=true;
            emit updateTitle(filename_, true);
        }
    }
}



void ISCADModel::unsetUnsavedState()
{
  unsaved_=false;
  emit updateTitle(filename_, false);
}


void ISCADModel::onScriptError(long failpos, QString errorMsg, int range)
{
  if (failpos>=0)
    {
      std::cout<<"moving cursor to "<<failpos<<" (l "<<range<<")"<<std::endl;
      QTextCursor tmpCursor = textCursor();
      tmpCursor.setPosition(failpos);
      tmpCursor.setPosition(failpos+range, QTextCursor::KeepAnchor);
      setTextCursor(tmpCursor);
    }
  else
    {
      std::cout<<"no error location info"<<std::endl;
    }

  if (bgparsethread_.action() < BGParsingThread::Rebuild)
    {
      emit displayStatusMessage("Script error: "+errorMsg);
    }
  else
    {
      QMessageBox::critical
          (
            this,
            "Script Error",
            errorMsg
          );
    }
}

void ISCADModel::onCancelRebuild()
{
  if (bgparsethread_.isRunning())
    {
      bgparsethread_.cancelRebuild();
    }
}

ISCADModelEditor::ISCADModelEditor(QWidget* parent)
: QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    QSplitter *spl=new QSplitter(Qt::Horizontal);
    layout->addWidget(spl);

    context_=new QoccViewerContext;
    viewer_=new QoccViewWidget(context_->getContext(), spl);
    spl->addWidget(viewer_);
    
    model_=new ISCADModel(spl);
    spl->addWidget(model_);

    QSplitter* spl2=new QSplitter(Qt::Vertical, spl);
    QGroupBox *gb;
    QVBoxLayout *vbox;


    gb=new QGroupBox("Controls");
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
//    gb->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
    spl2->addWidget(gb);

    gb=new QGroupBox("Model Tree");
    vbox = new QVBoxLayout;
    modeltree_=new QModelTree(gb);
    modeltree_->setMinimumHeight(20);
    vbox->addWidget(modeltree_);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Notepad");
    vbox = new QVBoxLayout;
    notepad_=new QTextEdit;

    vbox->addWidget(notepad_);
    QPushButton* copybtn=new QPushButton("<< Copy to cursor <<");
    vbox->addWidget(copybtn);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    spl->addWidget(spl2);
    
    {
      QList<int> sizes;
      sizes << 4700 << 3500 << 1700;
      spl->setSizes(sizes);
    }

    {
      QList<int> sizes;
      sizes << 2000 << 6000 << 2000;
      spl2->setSizes(sizes);
    }


    connect(rebuildBtn, SIGNAL(clicked()),
            model_, SLOT(rebuildModel()));
    connect(rebuildBtnUTC, SIGNAL(clicked()),
            model_, SLOT(rebuildModelUpToCursor()));
    connect(toggleBgParse, SIGNAL(stateChanged(int)),
            model_, SLOT(toggleBgParsing(int)));
    connect(toggleSkipPostprocActions, SIGNAL(stateChanged(int)),
            model_, SLOT(toggleSkipPostprocActions(int)));

    connect(copybtn, SIGNAL(clicked()),
            this, SLOT(onCopyBtnClicked()));

    model_->connectModelTree(modeltree_);

    connect(modeltree_, SIGNAL(show(QDisplayableModelTreeItem*)),
            viewer_, SLOT(onShow(QDisplayableModelTreeItem*)));
    connect(modeltree_, SIGNAL(hide(QDisplayableModelTreeItem*)),
            viewer_, SLOT(onHide(QDisplayableModelTreeItem*)));
    connect(modeltree_, SIGNAL(setDisplayMode(QDisplayableModelTreeItem*, AIS_DisplayMode)),
            viewer_, SLOT(onSetDisplayMode(QDisplayableModelTreeItem*, AIS_DisplayMode)));
    connect(modeltree_, SIGNAL(setColor(QDisplayableModelTreeItem*, Quantity_Color)),
            viewer_, SLOT(onSetColor(QDisplayableModelTreeItem*, Quantity_Color)));
    connect(modeltree_, SIGNAL(setResolution(QDisplayableModelTreeItem*, double)),
            viewer_, SLOT(onSetResolution(QDisplayableModelTreeItem*, double)));

    connect(model_, SIGNAL(updateTitle(boost::filesystem::path,bool)),
            this, SLOT(onUpdateTitle(boost::filesystem::path,bool)));

    connect(viewer_, SIGNAL(addEvaluationToModel(QString,insight::cad::PostprocActionPtr, bool)),
            modeltree_, SLOT(onAddEvaluation(QString,insight::cad::PostprocActionPtr, bool)));

    connect(viewer_, SIGNAL(insertNotebookText(QString)),
            this, SLOT(onInsertNotebookText(QString)));

}



void ISCADModelEditor::onCopyBtnClicked()
{
  model_->textCursor().insertText(notepad_->toPlainText());
  notepad_->clear();
}


void ISCADModelEditor::onInsertNotebookText(const QString& text)
{
  notepad_->insertPlainText(text);
}


void ISCADModelEditor::onUpdateTitle(const boost::filesystem::path& filepath, bool isUnsaved)
{
  emit updateTabTitle(this, filepath, isUnsaved);
}


void ISCADModelEditor::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::Yes;

    if (model_->isUnsaved())
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
                bool saved = model_->saveModel();
                if (!saved)
                {
                    saved=model_->saveModelAs();
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
