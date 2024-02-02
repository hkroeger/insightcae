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

#include "iqcaditemmodel.h"
#include "iqiscadmodelscriptedit.h"
#include "iqiscadsyntaxhighlighter.h"

//#include "qmodeltree.h"
//#include "qmodelstepitem.h"
//#include "qvariableitem.h"
//#include "qdatumitem.h"
//#include "qevaluationitem.h"
#include "iqiscadsyntaxhighlighter.h"
//#include "occtools.h"

//#include "base/boost_include.h"

#include <QDebug>
#include <QAction>
#include <QTimer>
#include <QMenu>
#include <QSignalMapper>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

#include "base/tools.h"
//#include "base/qt5_helper.h"
#include "base/translations.h"

#include "cadfeatures/modelfeature.h"
#include "sketch.h"
#include "modelcomponentselectordlg.h"
#include "insertfeaturedlg.h"
#include "loadmodeldialog.h"

#include "datum.h"






IQISCADModelScriptEdit::IQISCADModelScriptEdit(QWidget* parent, bool dobgparsing)
: QTextEdit(parent),
  unsaved_(false),
  doBgParsing_(dobgparsing),
  bgparsethread_(),
  skipPostprocActions_(true),
  cur_model_(nullptr)
{

    QFont defaultFont("Courier New");
    document()->setDefaultFont(defaultFont);
    setCurrentFont(defaultFont);

    // this fixes issue with font info vanishing in empty doc
    QTextCursor editorCursor = textCursor();
    editorCursor.movePosition(QTextCursor::Start);
    setTextCursor(editorCursor);

    setContextMenuPolicy(Qt::CustomContextMenu);

    QSettings settings("silentdynamics", "iscad");
    auto fs=settings.value("fontSize", this->font().pointSize()).toInt();

    setFontSize(fs);

    auto f=this->font();
    f.setPointSize(fs);
    QFontMetrics fm(f);
    sizehint_=QSize(
          3*fm.horizontalAdvance("abcdefghijklmnopqrstuvwxyz_-+*"),
          fm.height()
        );

    
    connect
        (
          this, &IQISCADModelScriptEdit::selectionChanged,
          this, &IQISCADModelScriptEdit::onEditorSelectionChanged
        );

    connect
        (
          this, &IQISCADModelScriptEdit::customContextMenuRequested,
          this, &IQISCADModelScriptEdit::showEditorContextMenu
        );

    highlighter_=new IQISCADSyntaxHighlighter(document());

    connect(&bgparsethread_, &IQISCADBackgroundThread::finished,
            this, &IQISCADModelScriptEdit::onBgParseFinished);
    connect(&bgparsethread_, &IQISCADBackgroundThread::statusMessage,
            this, &IQISCADModelScriptEdit::displayStatusMessage);
    connect(&bgparsethread_, &IQISCADBackgroundThread::statusProgress,
            this, &IQISCADModelScriptEdit::statusProgress);
    connect(&bgparsethread_, &IQISCADBackgroundThread::scriptError,
            this, &IQISCADModelScriptEdit::onScriptError);
    bgparseTimer_=new QTimer(this);
    connect(bgparseTimer_, &QTimer::timeout, this, &IQISCADModelScriptEdit::doBgParse);
    restartBgParseTimer();

    connect(document(), &QTextDocument::contentsChange,
            this, &IQISCADModelScriptEdit::restartBgParseTimer);
    connect(document(), &QTextDocument::contentsChange,
            this, &IQISCADModelScriptEdit::setUnsavedState);

}


IQISCADModelScriptEdit::~IQISCADModelScriptEdit()
{
    bgparsethread_.quit();
    bgparsethread_.wait();
}



bool IQISCADModelScriptEdit::saveModel()
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




bool IQISCADModelScriptEdit::saveModelAs()
{
    QString fn=QFileDialog::getSaveFileName(
        this, _("Select location"),
        "",
        _("ISCAD Model Files (*.iscad)") );
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




void IQISCADModelScriptEdit::setFontSize(int fontSize)
{
  fontSize_=std::max(2, fontSize);

  auto f=document()->defaultFont();
  f.setPointSize(fontSize_);
  document()->setDefaultFont(f);

  QTextCursor cursor = textCursor();
  selectAll();
  setCurrentFont(f);
  setTextCursor( cursor );

  QSettings settings("silentdynamics", "iscad");
  settings.setValue("fontSize", fontSize_);
}


void IQISCADModelScriptEdit::clearDerivedData()
{
  clear();
  emit clearData();
}


void IQISCADModelScriptEdit::loadFile(const boost::filesystem::path& file)
{
    if (!boost::filesystem::exists(file))
    {
        QMessageBox::critical(
            this, _("File error"),
            QString(_("The file %1 does not exists!")).arg(QString::fromStdString(file.string()))
            );
        return;
    }

    clearDerivedData();

    setFilename(file);
    std::string contents_raw;
    insight::readFileIntoString(file, contents_raw);

    setScript(contents_raw);
}


void IQISCADModelScriptEdit::setScript(const std::string& contents)
{
    clearDerivedData();

    disconnect
        (
          document(), &QTextDocument::contentsChange,
          this, &IQISCADModelScriptEdit::setUnsavedState
        );

    setPlainText(contents.c_str());

    connect
        (
          document(), &QTextDocument::contentsChange,
          this, &IQISCADModelScriptEdit::setUnsavedState
        );
}


void IQISCADModelScriptEdit::onEditorSelectionChanged()
{
    disconnect
    (
      this, &IQISCADModelScriptEdit::selectionChanged,
      this, &IQISCADModelScriptEdit::onEditorSelectionChanged
    );

    QString word=textCursor().selectedText();
    if ( !( word.contains('|') || word.contains('*') ) )
    {
        highlighter_->setHighlightWord(word);
        highlighter_->rehighlight();
    }

    if (syn_elem_dir_)
    {
      insight::cad::FeaturePtr fp=syn_elem_dir_->findElement( textCursor().position() );
      if (fp)
      {
        emit focus(fp);
      }
      else
      {
        emit unfocus();
      }
    }

    connect
    (
      this, &IQISCADModelScriptEdit::selectionChanged,
      this, &IQISCADModelScriptEdit::onEditorSelectionChanged
    );
}




void IQISCADModelScriptEdit::jumpTo(const QString& name)
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




void IQISCADModelScriptEdit::restartBgParseTimer(int,int,int)
{
    bgparseTimer_->setSingleShot(true);
    bgparseTimer_->start(bgparseInterval);
}



void IQISCADModelScriptEdit::doBgParse()
{
    if (doBgParsing_)
    {
        if (!bgparsethread_.isRunning())
        {
            emit displayStatusMessage(_("Background model parsing in progress..."));
            bgparsethread_.launch(
                        toPlainText().toStdString(),
                        IQISCADScriptModelGenerator::Parse
                        );
        }
    }
}

void IQISCADModelScriptEdit::onBgParseFinished()
{
    if (bgparsethread_.syn_elem_dir_)
    {
//        cur_model_ = bgparsethread_.model_;
        syn_elem_dir_ = bgparsethread_.syn_elem_dir_;

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




//void IQISCADModelScriptEdit::populateClipPlaneMenu(QMenu* clipplanemenu, QoccViewWidget* viewer)
//{
//    clipplanemenu->clear();
    
//    if (cur_model_)
//    {
//        int added=0;
//        insight::cad::Model::DatumTableContents datums = cur_model_->datums();
//        for (insight::cad::Model::DatumTableContents::value_type const& v: datums)
//        {
//            insight::cad::DatumPtr d = v.second;
//            if (d->providesPlanarReference())
//            {
//                QAction *act = new QAction( v.first.c_str(), this );
                
////                QSignalMapper *signalMapper = new QSignalMapper(this);
////                signalMapper->setMapping( act, reinterpret_cast<QObject*>(d.get()) );
////                connect
////                    (
////                      signalMapper, QOverload<QObject*>::of(&QSignalMapper::mapped),
////                      viewer, &QoccViewWidget::onSetClipPlane
////                    );
//                connect
//                    (
//                      act, &QAction::triggered,
////                      signalMapper, QOverload<>::of(&QSignalMapper::map)
//                      [d,&viewer]()
//                      {
//                        viewer->onSetClipPlane(reinterpret_cast<QObject*>(d.get()));
//                      }
//                    );
//                clipplanemenu->addAction(act);
                
//                added++;
//            }
//        }
        
//        if (added)
//        {
//            clipplanemenu->setEnabled(true);
//        }
//        else
//        {
//            clipplanemenu->setDisabled(true);
//        }
//    }
//}



//void IQISCADModelScriptEdit::connectModelTree(QModelTree* mt) const
//{
//  connect(&bgparsethread_, QOverload<const QString&,insight::cad::ScalarPtr>::of(&BGParsingThread::createdVariable),
//          mt, &QModelTree::onAddScalar);
//  connect(&bgparsethread_, QOverload<const QString&,insight::cad::VectorPtr,insight::cad::VectorVariableType>::of(&BGParsingThread::createdVariable),
//          mt, &QModelTree::onAddVector);
//  connect(&bgparsethread_, &IQISCADBackgroundThread::createdFeature,
//          mt, &QModelTree::onAddFeature);
//  connect(&bgparsethread_, &IQISCADBackgroundThread::createdDatum,
//          mt, &QModelTree::onAddDatum);
//  connect(&bgparsethread_, &IQISCADBackgroundThread::createdEvaluation,
//          mt, &QModelTree::onAddEvaluation );

//  connect(&bgparsethread_, &IQISCADBackgroundThread::removedScalar,
//          mt, &QModelTree::onRemoveScalar);
//  connect(&bgparsethread_, &IQISCADBackgroundThread::removedVector,
//          mt, &QModelTree::onRemoveVector);
//  connect(&bgparsethread_, &IQISCADBackgroundThread::removedFeature,
//          mt, &QModelTree::onRemoveFeature);
//  connect(&bgparsethread_, &BGParsingThread::removedDatum,
//          mt, &QModelTree::onRemoveDatum);
//  connect(&bgparsethread_, &BGParsingThread::removedEvaluation,
//          mt, &QModelTree::onRemoveEvaluation);

//  connect(mt, &QModelTree::jumpTo,
//          this, &IQISCADModelScriptEdit::jumpTo);
//  connect(mt, &QModelTree::insertParserStatementAtCursor,
//          this, &IQISCADModelScriptEdit::insertTextAtCursor);

//}

void IQISCADModelScriptEdit::setModel(IQCADItemModel* model)
{
    cur_model_=model;
    connect(cur_model_, &IQCADItemModel::jumpToDefinition,
            this, &IQISCADModelScriptEdit::jumpTo);
    connect(cur_model_, &IQCADItemModel::insertParserStatementAtCursor,
            this, &IQISCADModelScriptEdit::insertTextAtCursor);
    bgparsethread_.setModel(model);
}


QSize IQISCADModelScriptEdit::sizeHint() const
{
  return sizehint_;
}



void IQISCADModelScriptEdit::editSketch(QObject* skPtr)
{
    auto sk = reinterpret_cast<insight::cad::Sketch*>(skPtr);
    sk->executeEditor();
}


void IQISCADModelScriptEdit::editModel(QObject* mPtr)
{
    auto sk = reinterpret_cast<insight::cad::ModelFeature*>(mPtr);
    emit openModel(sk->modelfile());
}




void IQISCADModelScriptEdit::toggleBgParsing(int state)
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


void IQISCADModelScriptEdit::toggleSkipPostprocActions(int state)
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


void IQISCADModelScriptEdit::insertSectionCommentAtCursor()
{
    textCursor().insertText("\n\n############################################################\n#####  ");
}

void IQISCADModelScriptEdit::insertFeatureAtCursor()
{
    InsertFeatureDlg *dlg=new InsertFeatureDlg(this);
    if ( dlg->exec() == QDialog::Accepted )
    {
        textCursor().insertText(dlg->insert_string_);
    }
}

void IQISCADModelScriptEdit::insertImportedModelAtCursor()
{
  auto fn = QFileDialog::getOpenFileName(
        this, _("Please select file"), "",
        QString("%1 (*.step *.stp);;%2 (*.igs *.iges);;%3 (*.brep)")
            .arg(_("STEP model")).arg(_("IGES model")).arg(_("BREP model"))
        );
  if (!fn.isEmpty())
  {
    textCursor().insertText(QString("import(\"%1\")").arg(fn));
  }
}


void IQISCADModelScriptEdit::insertComponentNameAtCursor()
{
#warning reimplement!
//    auto dlg=new ModelComponentSelectorDlg(cur_model_, this);
//    if ( dlg->exec() == QDialog::Accepted )
//    {
//        std::string id = dlg->selected();
//        textCursor().insertText(id.c_str());
//    }
}

void IQISCADModelScriptEdit::insertLibraryModelAtCursor()
{
  auto dlg = new LoadModelDialog(this);
  if ( dlg->exec() == QDialog::Accepted )
  {
    std::string expr = dlg->expression();
    if (!expr.empty())
    {
      textCursor().insertText(expr.c_str());
    }
  }
}


void IQISCADModelScriptEdit::onIncreaseFontSize()
{
  setFontSize(++fontSize_);
}

void IQISCADModelScriptEdit::onDecreaseFontSize()
{
  setFontSize(--fontSize_);
}




void IQISCADModelScriptEdit::rebuildModel(bool upToCursor)
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
              skipPostprocActions_ ?
                        IQISCADScriptModelGenerator::Rebuild :
                        IQISCADScriptModelGenerator::Post
            );

    }
    else
    {
        emit displayStatusMessage(
            _("Background model parsing in progress, rebuild is currently disabled!"));
    }
}


void IQISCADModelScriptEdit::rebuildModelUpToCursor()
{
    rebuildModel(true);
}

void IQISCADModelScriptEdit::clearCache()
{
    insight::cad::cache.clear();
}




void IQISCADModelScriptEdit::showEditorContextMenu(const QPoint& pt)
{
    QMenu * menu = createStandardContextMenu();

    if (syn_elem_dir_)
    {
        QTextCursor cursor = cursorForPosition(pt);

        std::size_t po=/*editor_->textCursor()*/cursor.position();
        insight::cad::FeaturePtr fp=syn_elem_dir_->findElement(po);
        if (fp)
        {
//            QSignalMapper *signalMapper = new QSignalMapper(this);
            QAction *act=nullptr;

            insight::cad::Feature* fpp=fp.get();

            std::function<void()> slotFunction;
            if (insight::cad::Sketch* sk=dynamic_cast<insight::cad::Sketch*>(fpp))
            {
                act=new QAction(_("Edit Sketch..."), this);
                slotFunction=[this,sk]()
                {
                  this->editSketch(reinterpret_cast<QObject*>(sk));
                };
//                signalMapper->setMapping(act, reinterpret_cast<QObject*>(sk));
//                connect(signalMapper, QOverload<QObject*>::of(&QSignalMapper::mapped),
//                        this, &ISCADModel::editSketch);
            }
            else if (insight::cad::ModelFeature* mo=dynamic_cast<insight::cad::ModelFeature*>(fpp))
            {
                act=new QAction(_("Edit Model..."), this);
                slotFunction=[this,mo]()
                {
                   this->editModel(reinterpret_cast<QObject*>(mo));
                };
//                signalMapper->setMapping(act, reinterpret_cast<QObject*>(mo));
//                connect(signalMapper, QOverload<QObject*>::of(&QSignalMapper::mapped),
//                        this, &ISCADModel::editModel);
            }
            else
            {
//                 std::cout<<"NO"<<std::endl;
            }

            if (act)
            {
                connect(act, &QAction::triggered,
                        /*signalMapper, QOverload<>::of(&QSignalMapper::map)*/
                        slotFunction );
                menu->addSeparator();
                menu->addAction(act);
            }
        }
    }

    menu->exec(mapToGlobal(pt));
}


void IQISCADModelScriptEdit::insertTextAtCursor(const QString& snippet)
{
    textCursor().insertText(snippet);
}


void IQISCADModelScriptEdit::setUnsavedState(int, int rem, int ad)
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



void IQISCADModelScriptEdit::unsetUnsavedState()
{
  unsaved_=false;
  emit updateTitle(filename_, false);
}


void IQISCADModelScriptEdit::onScriptError(long failpos, QString errorMsg, int range)
{
  if (failpos>=0)
    {
      insight::dbg()<<"moving cursor to "<<failpos<<" (l "<<range<<")"<<std::endl;
      QTextCursor tmpCursor = textCursor();
      tmpCursor.setPosition(failpos);
      tmpCursor.setPosition(failpos+range, QTextCursor::KeepAnchor);
      setTextCursor(tmpCursor);
    }
  else
    {
      insight::dbg()<<"no error location info"<<std::endl;
    }

  if (bgparsethread_.finalTask()
          < IQISCADScriptModelGenerator::Rebuild)
    {
      emit displayStatusMessage(QString(_("Script error"))+": "+errorMsg);
    }
  else
    {
      QMessageBox::critical
          (
            this,
          _("Script Error"),
            errorMsg
          );
    }
}

void IQISCADModelScriptEdit::onCancelRebuild()
{
  if (bgparsethread_.isRunning())
    {
      bgparsethread_.cancelRebuild();
    }
}
