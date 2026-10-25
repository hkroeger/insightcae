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

#include "cadtypes.h"
#include "iqcaditemmodel.h"
#include "iqiscadmodelscriptedit.h"
#include "iqiscadsyntaxhighlighter.h"
#include "insertdrawingdialog.h"

//#include "qmodeltree.h"
//#include "qmodelstepitem.h"
//#include "qvariableitem.h"
//#include "qdatumitem.h"
//#include "qevaluationitem.h"
//#include "occtools.h"
#include "iqiscadfoldingmargin.h"

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
#include <QPointer>
#include <QApplication>
#include <QClipboard>
#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QKeyEvent>
#include <QToolTip>
#include <QAbstractItemView>
#include <QRegularExpression>

#include <sstream>

#include "base/tools.h"
#include "base/exceptionhandling.h"
//#include "base/qt5_helper.h"
#include "base/translations.h"

#include "cadfeature.h"
#include "cadfeatures/modelfeature.h"
#include "sketch.h"
#include "constrainedsketch.h"
#include "cadfeatures/parametricsketch.h"
#include "iqcadmodel3dviewer.h"
#include "iqvtkconstrainedsketcheditor.h"
#include "modelcomponentselectordlg.h"
#include "insertfeaturedlg.h"
#include "loadmodeldialog.h"

#include "datum.h"

#include "qtextensions.h"




IQISCADModelScriptEdit::IQISCADModelScriptEdit(QWidget* parent, bool dobgparsing)
: QPlainTextEdit(parent),
  highlighter_(nullptr),
  margin_(nullptr),
  completer_(nullptr),
  unsaved_(false),
  cur_model_(nullptr),
  viewer_(nullptr),
  skipPostprocActions_(true),
  hasBeenRebuilt_(false),
  inSelectionChanged_(false)
{

    QFont defaultFont("Courier New");
    defaultFont.setStyleHint(QFont::TypeWriter);
    document()->setDefaultFont(defaultFont);
    setFont(defaultFont);

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

    margin_=new IQISCADFoldingMargin(this);
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &IQISCADModelScriptEdit::updateMarginWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &IQISCADModelScriptEdit::updateMarginArea);
    updateMarginWidth();

    completer_=new IQISCADCompleter(this);
    completer_->setWidget(this);
    connect(completer_, QOverload<const QModelIndex&>::of(&QCompleter::activated),
            this, &IQISCADModelScriptEdit::insertCompletion);
    updateKnownCommands();

    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, [this]()
            {
                revealCursor();
                updateCalltip();
            });

    // adapt positions of tokens and fold regions
    connect(document(), &QTextDocument::contentsChange,
            this, &IQISCADModelScriptEdit::onContentsChange);

    scheduler_ = new IQISCADScriptJobScheduler(
        [this]() { return toPlainText().toStdString(); },
        nullptr,
        bgparseInterval,
        this );
    scheduler_->setBackgroundParsingEnabled(dobgparsing);

    connect(scheduler_, &IQISCADScriptJobScheduler::jobStarted,
            this, [this](IQISCADScriptModelGenerator::Task task, bool)
            {
                if (task==IQISCADScriptModelGenerator::Parse)
                    Q_EMIT displayStatusMessage(_("Background model parsing in progress..."));
            });
    connect(scheduler_, &IQISCADScriptJobScheduler::waitingForBackgroundParse,
            this, [this]()
            {
                Q_EMIT displayStatusMessage(
                    _("Waiting for background parsing to finish, rebuild will start afterwards..."));
            });
    connect(scheduler_, &IQISCADScriptJobScheduler::parsed,
            this, &IQISCADModelScriptEdit::onScriptParsed);
    connect(scheduler_, &IQISCADScriptJobScheduler::statusMessage,
            this, &IQISCADModelScriptEdit::displayStatusMessage);
    connect(scheduler_, &IQISCADScriptJobScheduler::statusProgress,
            this, &IQISCADModelScriptEdit::statusProgress);
    connect(scheduler_, &IQISCADScriptJobScheduler::jobEnded,
            this, [this](IQISCADScriptModelGenerator::Task)
            {
                Q_EMIT statusProgress(1, 1);
            });
    connect(scheduler_, &IQISCADScriptJobScheduler::scriptError,
            this, &IQISCADModelScriptEdit::onScriptError);
    connect(scheduler_, &IQISCADScriptJobScheduler::modelRebuilt,
            this, [this]() {
            if (!hasBeenRebuilt_)
            {
                Q_EMIT displayNeedsRefit();
                hasBeenRebuilt_=true;
            }
    });

    // cancel running background parse and restart after a while
    connect(document(), &QTextDocument::contentsChange,
            scheduler_, &IQISCADScriptJobScheduler::requestUpdate);
    connect(document(), &QTextDocument::contentsChange,
            this, &IQISCADModelScriptEdit::setUnsavedState);

}


IQISCADModelScriptEdit::~IQISCADModelScriptEdit()
{
    // stop all jobs, before the editor contents vanish
    delete scheduler_;
}



bool IQISCADModelScriptEdit::writeScriptTo(const boost::filesystem::path& fn)
{
    std::ofstream out(fn.c_str());
    out << toPlainText().toStdString();
    out.close();

    if (!out.good())
    {
        QMessageBox::critical(
            this, _("Save failed"),
            QString(_("The model could not be written to file %1!"))
                .arg(QString::fromStdString(fn.string()))
            );
        return false;
    }
    return true;
}




bool IQISCADModelScriptEdit::saveModel()
{
    if (filename_.empty())
    {
        return false;
    }

    if (!writeScriptTo(filename_))
    {
        return false;
    }

    unsetUnsavedState();
    return true;
}




// TODO: this persists the raw editor text verbatim and never rebases any
// relative PathParameters embedded in sketch entities onto the new
// directory (see insight::Parameter::resolveRelativePaths). Harmless today
// since no shipped ConstrainedSketchEntity has a file-backed parameter, but
// must be revisited (regenerate the script via generateScriptCommand()
// after calling resolveRelativePaths() on the affected parameter sets)
// before adding one.
bool IQISCADModelScriptEdit::saveModelAs()
{
    if (auto fn = getFileName(
        this, _("Select location"),
        GetFileMode::Save,
        {{ "iscad", _("ISCAD Model Files") }} ) )
    {
        // keep the previous file name, if the new file cannot be written
        if (!writeScriptTo(fn))
        {
            return false;
        }

        setFilename(fn);
        unsetUnsavedState();
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
  setFont(f);

  if (margin_)
  {
    margin_->setFont(f);
    updateMarginWidth();
  }

  QSettings settings("silentdynamics", "iscad");
  settings.setValue("fontSize", fontSize_);
}


void IQISCADModelScriptEdit::clearDerivedData()
{
  // output of running jobs belongs to the old script
  scheduler_->stop();
  scheduler_->discardParseResult();
  clear();
  syn_elem_dir_.reset();
  syn_elem_pos_=ScriptPositionMap();
  highlighter_->setErrorRange(-1, -1);
  calltipCommand_.clear();
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

    setFilename(file);
    std::string contents_raw;
    insight::readFileIntoString(file, contents_raw);

    setScript(contents_raw);
}


void IQISCADModelScriptEdit::setScript(const std::string& contents)
{
    clearDerivedData();
    hasBeenRebuilt_=false;

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
    if (inSelectionChanged_)
        return;

    // reset also in case of exceptions
    struct Guard
    {
        bool& flag;
        Guard(bool& f) : flag(f) { flag=true; }
        ~Guard() { flag=false; }
    } guard(inSelectionChanged_);

    highlightOccurrences(textCursor().selectedText());

    if (syn_elem_dir_)
    {
        bool somethingFocussed=false;
      auto sde=syn_elem_dir_->findElement( parserPosition(textCursor().position()) );

      // the entity is built on demand for display. Build errors must not
      // propagate: cursor movement should not raise dialogs
      auto tryFocus = [&](std::function<insight::cad::FeaturePtr()> getFeature)
      {
          try
          {
              Q_EMIT focus(getFeature());
              somethingFocussed=true;
          }
          catch (...)
          {
              auto desc = insight::describeCurrentException();
              Q_EMIT displayStatusMessage(
                  QString(_("Could not display the selected entity: %1"))
                      .arg(QString::fromStdString(std::string(*desc))) );
          }
      };

      if (auto fp=boost::get<insight::cad::FeaturePtr>(&sde))
      {
          if (*fp)
          {
            auto f=*fp;
            tryFocus([f]() { return f; });
          }
      }
      else if (auto fsp=boost::get<insight::cad::FeatureSetPtr>(&sde))
      {
          if (*fsp)
          {
            auto fs=*fsp;
            tryFocus([fs]() { return insight::cad::Import::create(fs); });
          }
      }

      if (!somethingFocussed)
      {
        Q_EMIT unfocus();
      }
    }
}




void IQISCADModelScriptEdit::highlightOccurrences(const QString& word)
{
    QList<QTextEdit::ExtraSelection> selections;

    static const QRegularExpression identifier("^[A-Za-z_][A-Za-z0-9_]*$");
    if (identifier.match(word).hasMatch())
    {
        QTextCharFormat fmt;
        fmt.setBackground(Qt::yellow);

        QRegularExpression re("\\b"+word+"\\b");
        QTextCursor c(document());
        const int maxOccurrences=1000;
        while (selections.size()<maxOccurrences)
        {
            c=document()->find(re, c);
            if (c.isNull()) break;
            QTextEdit::ExtraSelection sel;
            sel.cursor=c;
            sel.format=fmt;
            selections.append(sel);
        }
    }

    setExtraSelections(selections);
}


void IQISCADModelScriptEdit::jumpTo(const QString& name)
{
    highlightOccurrences(name);

    QRegularExpression expression("\\b("+QRegularExpression::escape(name)+")\\b");
    int i=toPlainText().indexOf(expression);

    if (i>=0)
    {
        QTextCursor tmpCursor = textCursor();
        tmpCursor.setPosition(i);
        setTextCursor(tmpCursor);
        centerCursor();
    }
}




void IQISCADModelScriptEdit::onScriptParsed(
    ISCADParseResultPtr parseResult,
    IQISCADScriptModelGenerator::Task )
{
    if (!parseResult->syntaxElements)
        return;

    // the result is outdated, if the script has been changed in the meantime
    // (a newer parse is scheduled then).
    // The parsed script may be a part of the editor content (rebuild up to cursor).
    const auto& script = parseResult->script;
    std::string current = toPlainText().toStdString();
    if (current.compare(0, script.size(), script)!=0)
        return;

    ScriptPositionMap pm(current);
    const auto& sd = *parseResult->syntaxElements;

    syn_elem_dir_ = parseResult->syntaxElements;
    syn_elem_pos_ = pm;
    updateKnownCommands();

    // range of the script, for which the parse result is valid
    long validBytes = script.size();
    if (!parseResult->success)
    {
        if (parseResult->failpos>=0)
        {
            validBytes = parseResult->failpos;
        }
        else
        {
            // no location information: trust everything that was recognized
            validBytes = 0;
            for (const auto& t: sd.tokens)
                validBytes = std::max(validBytes, t.second.range.second);
            for (const auto& f: sd.foldRanges)
                validBytes = std::max(validBytes, f.second.range.second);
        }
    }
    updateAnnotations(sd, pm, pm.toChar(validBytes));

    if (!parseResult->success && parseResult->failpos>=0)
    {
        highlighter_->setErrorRange(
            pm.toChar(parseResult->failpos),
            pm.toChar(parseResult->failpos+std::max(1, parseResult->errorRange)) );
    }
    else
    {
        highlighter_->setErrorRange(-1, -1);
    }

    highlighter_->rehighlight();

    emit modelUpdated();
}




long IQISCADModelScriptEdit::parserPosition(int editorPosition) const
{
    return syn_elem_pos_.toByte(editorPosition);
}




void IQISCADModelScriptEdit::updateAnnotations(
    const insight::cad::parser::SyntaxElementDirectory& sd,
    const ScriptPositionMap& pm,
    int validChars )
{
    // ===== tokens
    std::vector<IQISCADToken> tokens;
    int lastEnd=0;
    for (const auto& t: sd.tokens)
    {
        int b=pm.toChar(t.second.range.first);
        int e=pm.toChar(t.second.range.second);
        if (b<validChars && e>b && b>=lastEnd)
        {
            tokens.push_back(IQISCADToken{b, e, t.second.kind});
            lastEnd=e;
        }
    }
    // keep the previous information behind the valid range
    for (const auto& t: highlighter_->tokens())
    {
        if (t.begin>=std::max(validChars, lastEnd))
        {
            tokens.push_back(t);
        }
    }
    highlighter_->setTokens(std::move(tokens));

    // ===== fold regions
    std::vector<FoldRegion> regions;
    for (const auto& f: sd.foldRanges)
    {
        FoldRegion r{
            pm.toChar(f.second.range.first),
            pm.toChar(f.second.range.second),
            QString::fromStdString(f.second.command),
            false };

        if (r.begin<validChars && r.end>r.begin)
        {
            // keep the state of previously known regions
            for (const auto& o: foldRegions_)
            {
                if (o.begin==r.begin && o.command==r.command)
                {
                    r.collapsed=o.collapsed;
                    break;
                }
            }
            regions.push_back(r);
        }
    }
    for (const auto& o: foldRegions_)
    {
        if (o.begin>=validChars)
            regions.push_back(o);
    }
    std::sort(regions.begin(), regions.end(),
              [](const FoldRegion& a, const FoldRegion& b)
              { return a.begin<b.begin || (a.begin==b.begin && a.end>b.end); } );
    foldRegions_=std::move(regions);

    applyFolding();
    margin_->update();
}




void IQISCADModelScriptEdit::onContentsChange(int pos, int removed, int added)
{
    highlighter_->adaptToContentsChange(pos, removed, added);

    const int delta=added-removed;
    bool collapsedRegionRemoved=false;
    std::vector<FoldRegion> adapted;
    adapted.reserve(foldRegions_.size());
    for (auto r: foldRegions_)
    {
        if (r.end<=pos)
        {
            adapted.push_back(r);
        }
        else if (r.begin>=pos+removed)
        {
            r.begin+=delta;
            r.end+=delta;
            adapted.push_back(r);
        }
        else if ( pos >= r.begin+std::max(1, int(r.command.size()))
                  && pos+removed <= r.end-1 )
        {
            // edit inside the argument list or block
            r.end+=delta;
            adapted.push_back(r);
        }
        else
        {
            // the begin or end of the region has been edited
            collapsedRegionRemoved = collapsedRegionRemoved || r.collapsed;
        }
    }
    foldRegions_=std::move(adapted);

    if (collapsedRegionRemoved)
    {
        // a later applyFolding is required (not while the document is being changed)
        QTimer::singleShot(0, this, &IQISCADModelScriptEdit::applyFolding);
    }
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
    hasBeenRebuilt_=false;
    cur_model_=model;
    connect(cur_model_, &IQCADItemModel::jumpToDefinition,
            this, &IQISCADModelScriptEdit::jumpTo);
    connect(cur_model_, &IQCADItemModel::insertParserStatementAtCursor,
            this, &IQISCADModelScriptEdit::insertTextAtCursor);
    connect(cur_model_, &IQCADItemModel::statusMessage,
            this, &IQISCADModelScriptEdit::displayStatusMessage);
    scheduler_->setModel(model);
}


void IQISCADModelScriptEdit::setViewer(IQCADModel3DViewer* viewer)
{
    viewer_=viewer;
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


void IQISCADModelScriptEdit::editParametricSketch(
    std::shared_ptr<insight::cad::ParametricSketch> ps )
{
    if (!viewer_) return;

    // the parser positions are byte offsets in the UTF-8 encoded script,
    // convert them into character positions in the editor
    std::string script = toPlainText().toStdString();
    auto range = ps->scriptRange();
    QString text = toPlainText();

    bool ok = (range.first<=range.second) && (range.second<=script.size());
    int b=0, e=0;
    if (ok)
    {
        ScriptPositionMap pm(script);
        b = pm.toChar(range.first);
        e = pm.toChar(range.second);
        // replace up to the closing bracket
        while (e<text.size() && text[e].isSpace()) ++e;
        ok = (e<text.size()) && (text[e]==')');
    }
    if (!ok)
    {
        QMessageBox::warning(
            this, _("Edit Parametric Sketch"),
            _("The script has been modified since it was parsed last."
              " Please wait for the background parsing to complete and try again.") );
        return;
    }

    if (ps->hasScriptArgument())
    {
        // replace everything after the comma
        int i=b;
        while (i>0 && text[i-1].isSpace()) --i;
        if (i>0 && text[i-1]==',') b=i;
    }

    QString orgText = text.mid(b, e-b);
    bool hasScriptArgument = ps->hasScriptArgument();

    QPointer<IQISCADModelScriptEdit> self(this);
    viewer_->editSketch(
        *ps->sketch(),
        insight::cad::noParametersDelegate,
        defaultGUIConstrainedSketchPresentationDelegate,
        [self,b,e,orgText,hasScriptArgument](insight::cad::ConstrainedSketchPtr editedSk)
        {
            if (self)
            {
                self->replaceParametricSketchScript(
                    b, e, orgText, hasScriptArgument, *editedSk );
            }
        }
        );
}


void IQISCADModelScriptEdit::replaceParametricSketchScript(
    int begin, int end,
    const QString& orgText,
    bool hasScriptArgument,
    const insight::cad::ConstrainedSketch& sketch )
{
    std::ostringstream os;
    sketch.generateScript(os);
    QString sketchScript = QString::fromStdString(os.str()).trimmed();

    QString replacement;
    if (sketchScript.isEmpty())
        replacement = hasScriptArgument ? " " : ", ";
    else
        replacement = (hasScriptArgument ? "\n" : ",\n") + sketchScript + "\n";

    if (toPlainText().mid(begin, end-begin) != orgText)
    {
        auto answer = QMessageBox::warning(
            this, _("Edit Parametric Sketch"),
            _("The script has been modified during sketch editing."
              " The sketch script cannot be inserted automatically.\n"
              "Copy the sketch script to the clipboard?"),
            QMessageBox::Yes|QMessageBox::No );
        if (answer==QMessageBox::Yes)
        {
            QApplication::clipboard()->setText(sketchScript);
        }
        return;
    }

    QTextCursor c = textCursor();
    c.beginEditBlock();
    c.setPosition(begin);
    c.setPosition(end, QTextCursor::KeepAnchor);
    c.insertText(replacement);
    c.endEditBlock();
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
        scheduler_->setBackgroundParsingEnabled(true);
    }
    else if (state==Qt::Unchecked)
    {
        scheduler_->setBackgroundParsingEnabled(false);
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
    if (auto fn = getFileName(
          this, _("Please select file"),
          GetFileMode::Open,
          {
            { "stp step", _("STEP model") },
            { "igs iges", _("IGES model") },
            { "brep", _("BREP model") }
          }))
  {
    textCursor()
            .insertText(
                QString("import(\"%1\")")
                    .arg(fn.asQString()));
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


void IQISCADModelScriptEdit::insertDrawingAtCursor()
{
    auto dlg = new InsertDrawingDialog(this);
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
    auto task =
        skipPostprocActions_ ?
            IQISCADScriptModelGenerator::Rebuild :
            IQISCADScriptModelGenerator::Post;

    if (upToCursor)
    {
        std::string script_content = toPlainText().toStdString();
        QTextCursor c = textCursor();
        script_content = script_content.substr(
            0, ScriptPositionMap(script_content).toByte(c.position()) );

        scheduler_->rebuildScript(script_content, task);
    }
    else
    {
        // reuses the result of the background parser, if possible
        scheduler_->rebuild(task);
    }
}


void IQISCADModelScriptEdit::rebuildModelUpToCursor()
{
    rebuildModel(true);
}

void IQISCADModelScriptEdit::clearCache()
{
    insight::cad::cache.clear();

    // the entities of the last parse result might be built already
    scheduler_->discardParseResult();
    scheduler_->requestUpdate();
}




void IQISCADModelScriptEdit::showEditorContextMenu(const QPoint& pt)
{
    QMenu * menu = createStandardContextMenu();

    if (syn_elem_dir_)
    {
        QTextCursor cursor = cursorForPosition(pt);

        auto se=syn_elem_dir_->findElement(parserPosition(cursor.position()));
        if (auto fpp=boost::get<insight::cad::FeaturePtr>(&se))
        {
//            QSignalMapper *signalMapper = new QSignalMapper(this);
            QAction *act=nullptr;

            insight::cad::Feature* fp=fpp->get();

            std::function<void()> slotFunction;
            if (insight::cad::Sketch* sk=dynamic_cast<insight::cad::Sketch*>(fp))
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
            else if (auto ps=std::dynamic_pointer_cast<insight::cad::ParametricSketch>(*fpp))
            {
                act=new QAction(_("Edit Parametric Sketch..."), this);
                act->setEnabled(viewer_!=nullptr);
                slotFunction=[this,ps]()
                {
                    this->editParametricSketch(ps);
                };
            }
            else if (insight::cad::ModelFeature* mo=dynamic_cast<insight::cad::ModelFeature*>(fp))
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

    menu->addSeparator();
    auto *foldMenu = menu->addMenu(_("Folding"));
    connect(foldMenu->addAction(_("Collapse all sketches")), &QAction::triggered,
            this, &IQISCADModelScriptEdit::collapseAllSketches);
    connect(foldMenu->addAction(_("Collapse all")), &QAction::triggered,
            this, &IQISCADModelScriptEdit::collapseAll);
    connect(foldMenu->addAction(_("Expand all")), &QAction::triggered,
            this, &IQISCADModelScriptEdit::expandAll);

    menu->exec(viewport()->mapToGlobal(pt));
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


void IQISCADModelScriptEdit::onScriptError(
    long failpos, QString errorMsg, int range,
    IQISCADScriptModelGenerator::Task task )
{
  if (failpos>=0)
    {
      insight::dbg()<<"moving cursor to "<<failpos<<" (l "<<range<<")"<<std::endl;
      // failpos and range are byte offsets in the script
      ScriptPositionMap pm(toPlainText().toStdString());
      QTextCursor tmpCursor = textCursor();
      tmpCursor.setPosition(pm.toChar(failpos));
      tmpCursor.setPosition(pm.toChar(failpos+range), QTextCursor::KeepAnchor);
      setTextCursor(tmpCursor);
    }
  else
    {
      insight::dbg()<<"no error location info"<<std::endl;
    }

  if (task < IQISCADScriptModelGenerator::Rebuild)
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
  if (!scheduler_->isIdle())
    {
      scheduler_->stop();
      emit displayStatusMessage(_("Model rebuild cancelled"));
      emit statusProgress(1, 1);
    }
}






// ================================================================
// folding
// ================================================================


int IQISCADModelScriptEdit::marginWidth() const
{
    int digits = 2;
    for (int m = qMax(1, blockCount()); m >= 100; m /= 10)
        ++digits;
    const QFontMetrics fm(font());
    return 6 + fm.horizontalAdvance(QLatin1Char('9')) * digits + fm.height();
}


void IQISCADModelScriptEdit::updateMarginWidth()
{
    setViewportMargins(marginWidth(), 0, 0, 0);
}


void IQISCADModelScriptEdit::updateMarginArea(const QRect &rect, int dy)
{
    if (dy)
        margin_->scroll(0, dy);
    else
        margin_->update(0, rect.y(), margin_->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateMarginWidth();
}


void IQISCADModelScriptEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    margin_->setGeometry(QRect(cr.left(), cr.top(), marginWidth(), cr.height()));
}


bool IQISCADModelScriptEdit::isFoldable(const FoldRegion& r) const
{
    auto b=document()->findBlock(r.begin);
    auto e=document()->findBlock(std::max(r.begin, r.end-1));
    // at least one line between first and last line
    return b.isValid() && e.isValid() && (e.blockNumber()-b.blockNumber()>=2);
}


bool IQISCADModelScriptEdit::hidesPosition(const FoldRegion& r, int pos) const
{
    if (!isFoldable(r)) return false;
    int s=document()->findBlock(r.begin).blockNumber();
    int e=document()->findBlock(r.end-1).blockNumber();
    int p=document()->findBlock(pos).blockNumber();
    return s<p && p<e;
}


int IQISCADModelScriptEdit::foldRegionStartingIn(const QTextBlock& block) const
{
    const int bb=block.position(), be=bb+block.length();
    // regions are sorted by begin: the first match is the outermost one
    for (int i=0; i<int(foldRegions_.size()); ++i)
    {
        const auto& r=foldRegions_[i];
        if (r.begin>=be) break;
        if (r.begin>=bb && isFoldable(r))
            return i;
    }
    return -1;
}


void IQISCADModelScriptEdit::applyFolding()
{
    auto *doc=document();

    std::vector<bool> hidden(doc->blockCount(), false);
    for (const auto& r: foldRegions_)
    {
        if (r.collapsed && isFoldable(r))
        {
            int s=doc->findBlock(r.begin).blockNumber();
            int e=doc->findBlock(r.end-1).blockNumber();
            for (int k=s+1; k<e && k<int(hidden.size()); ++k)
                hidden[k]=true;
        }
    }

    bool changed=false;
    for (auto b=doc->begin(); b!=doc->end(); b=b.next())
    {
        bool visible = !hidden[b.blockNumber()];
        if (b.isVisible()!=visible)
        {
            b.setVisible(visible);
            b.setLineCount(visible ? qMax(1, b.layout()->lineCount()) : 0);
            changed=true;
        }
    }

    if (changed)
    {
        doc->markContentsDirty(0, doc->characterCount());
        viewport()->update();
        margin_->update();
        ensureCursorVisible();
    }
}


void IQISCADModelScriptEdit::setCollapsed(int regionIndex, bool collapsed)
{
    auto& r=foldRegions_[regionIndex];
    if (collapsed)
    {
        // move the cursor out of the lines, which will be hidden
        auto c=textCursor();
        if (hidesPosition(r, c.position()))
        {
            auto startBlock=document()->findBlock(r.begin);
            c.setPosition(startBlock.position()+startBlock.length()-1);
            setTextCursor(c);
        }
    }
    r.collapsed=collapsed;
    applyFolding();
}


void IQISCADModelScriptEdit::setAllCollapsed(
    bool collapsed,
    const std::function<bool(const FoldRegion&)>& filter )
{
    for (int i=0; i<int(foldRegions_.size()); ++i)
    {
        auto& r=foldRegions_[i];
        if (filter(r) && isFoldable(r))
        {
            if (collapsed)
            {
                setCollapsed(i, true);
            }
            else
            {
                r.collapsed=false;
            }
        }
    }
    applyFolding();
}


void IQISCADModelScriptEdit::collapseAllSketches()
{
    setAllCollapsed(
        true,
        [](const FoldRegion& r)
        {
            return r.command=="ParametricSketch" || r.command=="ConstrainedSketch";
        });
}


void IQISCADModelScriptEdit::collapseAll()
{
    setAllCollapsed(true, [](const FoldRegion&) { return true; });
}


void IQISCADModelScriptEdit::expandAll()
{
    setAllCollapsed(false, [](const FoldRegion&) { return true; });
}


void IQISCADModelScriptEdit::revealCursor()
{
    if (textCursor().block().isVisible())
        return;

    const int p=textCursor().position();
    for (auto& r: foldRegions_)
    {
        if (r.collapsed && hidesPosition(r, p))
            r.collapsed=false;
    }
    applyFolding();
}


void IQISCADModelScriptEdit::marginPaintEvent(QPaintEvent *event)
{
    QPainter painter(margin_);
    painter.fillRect(event->rect(), QColor(240, 240, 240));

    const QFontMetrics fm(font());
    const int markerSize = fm.height();
    const int numberWidth = margin_->width() - markerSize - 4;

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            painter.setPen(Qt::gray);
            painter.drawText(
                0, top, numberWidth, fm.height(),
                Qt::AlignRight, QString::number(block.blockNumber() + 1) );

            int ri=foldRegionStartingIn(block);
            if (ri>=0)
            {
                const auto& r = foldRegions_[ri];
                QRectF mr(numberWidth+4, top, markerSize, fm.height());
                mr.adjust(markerSize*0.25, markerSize*0.25, -markerSize*0.25, -markerSize*0.25);
                QPolygonF tri;
                if (r.collapsed)
                {
                    tri << mr.topLeft() << QPointF(mr.right(), mr.center().y()) << mr.bottomLeft();
                }
                else
                {
                    tri << mr.topLeft() << mr.topRight() << QPointF(mr.center().x(), mr.bottom());
                }
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(90, 90, 90));
                painter.drawPolygon(tri);
                painter.setRenderHint(QPainter::Antialiasing, false);
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
}


void IQISCADModelScriptEdit::marginMousePressEvent(QMouseEvent *event)
{
    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->pos().y())
    {
        if (block.isVisible() && event->pos().y() < bottom)
        {
            int ri=foldRegionStartingIn(block);
            if (ri>=0)
            {
                setCollapsed(ri, !foldRegions_[ri].collapsed);
            }
            return;
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
    }
}


void IQISCADModelScriptEdit::paintEvent(QPaintEvent *e)
{
    QPlainTextEdit::paintEvent(e);

    // placeholders at the end of the first line of collapsed regions
    QPainter painter(viewport());
    const QFontMetrics fm(font());
    const QString ph(QChar(0x2026)); // ...

    QTextBlock lastBlock;
    for (const auto& r: foldRegions_)
    {
        if (!r.collapsed || !isFoldable(r)) continue;

        auto block=document()->findBlock(r.begin);
        if (!block.isVisible() || block==lastBlock) continue;
        lastBlock=block;

        QRectF geo = blockBoundingGeometry(block).translated(contentOffset());
        if (geo.bottom()<e->rect().top() || geo.top()>e->rect().bottom()) continue;

        auto *layout=block.layout();
        if (!layout || layout->lineCount()<1) continue;
        QRectF lr = layout->lineAt(layout->lineCount()-1).naturalTextRect()
                    .translated(geo.topLeft());

        QRectF box(lr.right()+6, lr.top()+1, fm.horizontalAdvance(ph)+8, lr.height()-2);
        painter.setPen(Qt::gray);
        painter.setBrush(QColor(235, 235, 235));
        painter.drawRoundedRect(box, 3, 3);
        painter.drawText(box, Qt::AlignCenter, ph);
    }
}




// ================================================================
// completion
// ================================================================


namespace {

inline bool isIdentifierChar(QChar c)
{
    return c.isLetterOrNumber() || c==QChar('_');
}

}


QString IQISCADModelScriptEdit::identifierPrefix() const
{
    auto c=textCursor();
    const QString t=c.block().text();
    int e=c.positionInBlock();
    int b=e;
    while (b>0 && isIdentifierChar(t[b-1])) --b;
    // identifiers do not start with a digit
    while (b<e && t[b].isDigit()) ++b;
    return t.mid(b, e-b);
}


bool IQISCADModelScriptEdit::cursorInCommentOrString() const
{
    auto c=textCursor();
    auto pb=c.block().previous();
    return IQISCADSyntaxHighlighter::isInCommentOrString(
        c.block().text(), c.positionInBlock(),
        pb.isValid() ? pb.userState() : IQISCADSyntaxHighlighter::Normal );
}


QString IQISCADModelScriptEdit::commandSignature(const QString& command)
{
    static std::map<QString, QString> signatures;
    if (signatures.empty() && insight::cad::Feature::ruleDocumentationFunctions_)
    {
        for (const auto& rd: *insight::cad::Feature::ruleDocumentationFunctions_)
        {
            for (const auto& info: rd.second())
            {
                signatures[QString::fromStdString(info.command_)] =
                    QString::fromStdString(info.signature_);
            }
        }
    }
    auto i=signatures.find(command);
    if (i!=signatures.end())
        return command+i->second;
    return QString();
}


void IQISCADModelScriptEdit::updateKnownCommands()
{
    auto& cmds=knownCommands_;
    cmds.clear();

    if (insight::cad::Feature::ruleDocumentationFunctions_)
    {
        for (const auto& rd: *insight::cad::Feature::ruleDocumentationFunctions_)
        {
            for (const auto& info: rd.second())
            {
                cmds[QString::fromStdString(info.command_)]=IQISCADCompleter::Command;
            }
        }
    }

    if (syn_elem_dir_)
    {
        using insight::cad::parser::CommandKind;
        for (const auto& c: syn_elem_dir_->commands)
        {
            IQISCADCompleter::CandidateKind k = IQISCADCompleter::Command;
            switch (c.second)
            {
                case CommandKind::FeatureCommand: k=IQISCADCompleter::Command; break;
                case CommandKind::ScalarFunction:
                case CommandKind::VectorFunction: k=IQISCADCompleter::Function; break;
                case CommandKind::PostprocCommand: k=IQISCADCompleter::PostprocCommand; break;
            }
            cmds[QString::fromStdString(c.first)]=k;
        }
    }
}


QString IQISCADModelScriptEdit::enclosingCommand(int pos) const
{
    const auto* doc=document();
    const auto& cmds=knownCommands_;

    // search backwards for an unmatched opening bracket
    int depth=0;
    const int limit=std::max(0, pos-20000);
    for (int i=std::min(pos, doc->characterCount()-1)-1; i>=limit; --i)
    {
        QChar c=doc->characterAt(i);
        if (c==QChar(')'))
        {
            ++depth;
        }
        else if (c==QChar('('))
        {
            if (depth>0)
            {
                --depth;
            }
            else
            {
                // the identifier in front of the bracket
                int e=i;
                while (e>0 && doc->characterAt(e-1).isSpace()) --e;
                int b=e;
                while (b>0 && isIdentifierChar(doc->characterAt(b-1))) --b;
                QString name;
                for (int k=b; k<e; ++k) name+=doc->characterAt(k);
                if (cmds.count(name))
                    return name;
                // e.g. a nested bracket expression: continue outside
            }
        }
        else if (c==QChar(';'))
        {
            // end of the previous statement (outside of submodel blocks)
            if (depth==0) break;
        }
    }
    return QString();
}


std::vector<IQISCADCompleter::Candidate>
IQISCADModelScriptEdit::completionCandidates(int pos) const
{
    using C=IQISCADCompleter;
    using insight::cad::parser::TokenKind;

    std::vector<C::Candidate> cands;
    const QString text=toPlainText();

    // keywords of the current command come first
    QString cmd=enclosingCommand(pos);
    if (!cmd.isEmpty())
    {
        QString sig=commandSignature(cmd);
        // keywords are the identifiers outside of <...> placeholders
        QString kws=sig.mid(cmd.size());
        kws.remove(QRegularExpression("<[^>\\]]*[>\\]]?"));
        auto it=QRegularExpression("[A-Za-z_][A-Za-z0-9_]*").globalMatch(kws);
        while (it.hasNext())
        {
            cands.push_back({it.next().captured(0), C::Keyword, _("keyword of ")+cmd});
        }
    }

    // commands
    for (const auto& c: knownCommands_)
    {
        cands.push_back({c.first, c.second, commandSignature(c.first)});
    }

    // predefined symbols
    for (const char* s: {"M_PI", "deg"})
        cands.push_back({s, C::Scalar, _("predefined scalar")});
    for (const char* s: {"O", "EX", "EY", "EZ"})
        cands.push_back({s, C::Vector, _("predefined vector")});
    for (const char* s: {"XY", "XZ", "YZ"})
        cands.push_back({s, C::Datum, _("predefined plane")});

    // submodel blocks, which contain the position
    std::vector<const FoldRegion*> enclosingBlocks;
    for (const auto& r: foldRegions_)
    {
        if (r.command.isEmpty() && r.begin<pos && pos<r.end)
            enclosingBlocks.push_back(&r);
    }

    // loop variables of enclosing for loops
    for (const auto* sb: enclosingBlocks)
    {
        for (const auto& r: foldRegions_)
        {
            if (r.command=="for" && r.begin<sb->begin && sb->end<=r.end)
            {
                static const QRegularExpression loopVar("^for\\s*\\(\\s*([A-Za-z_][A-Za-z0-9_]*)");
                auto m=loopVar.match(text.mid(r.begin, r.end-r.begin));
                if (m.hasMatch())
                    cands.push_back({m.captured(1), C::Scalar, _("loop variable")});
            }
        }
    }

    // defined symbols
    for (const auto& t: highlighter_->tokens())
    {
        if (t.begin>=pos) break;

        C::CandidateKind k;
        switch (t.kind)
        {
            case TokenKind::FeatureDef:
            case TokenKind::ComponentDef: k=C::Feature; break;
            case TokenKind::ScalarDef: k=C::Scalar; break;
            case TokenKind::VectorDef: k=C::Vector; break;
            case TokenKind::DatumDef: k=C::Datum; break;
            case TokenKind::SelectionDef: k=C::Selection; break;
            default: continue;
        }

        // symbols inside a submodel are only visible there
        const FoldRegion* scope=nullptr;
        for (const auto& r: foldRegions_)
        {
            if (r.command.isEmpty() && r.begin<t.begin && t.end<=r.end
                && (!scope || r.begin>scope->begin) )
                scope=&r;
        }
        if (scope && !(scope->begin<pos && pos<scope->end))
            continue;

        cands.push_back({text.mid(t.begin, t.end-t.begin), k, QString()});
    }

    return cands;
}


void IQISCADModelScriptEdit::showCompletion(bool explicitlyRequested)
{
    auto *popup=completer_->popup();

    if (cursorInCommentOrString())
    {
        popup->hide();
        return;
    }

    const QString prefix=identifierPrefix();

    if (!popup->isVisible() || explicitlyRequested)
    {
        completer_->setCandidates(completionCandidates(textCursor().position()));
    }

    completer_->setCompletionPrefix(prefix);

    if ( completer_->completionCount()==0
         || ( !explicitlyRequested && completer_->completionCount()==1
              && completer_->currentCompletion()==prefix ) )
    {
        popup->hide();
        return;
    }

    popup->setCurrentIndex(completer_->completionModel()->index(0, 0));

    QRect cr=cursorRect();
    cr.translate(viewport()->geometry().topLeft());
    cr.setWidth(std::min(
        700,
        popup->sizeHintForColumn(0) + popup->sizeHintForColumn(1)
            + popup->verticalScrollBar()->sizeHint().width() + 20 ));
    completer_->complete(cr);
}


void IQISCADModelScriptEdit::insertCompletion(const QModelIndex& index)
{
    const QString completion=index.sibling(index.row(), 0).data().toString();
    const auto kind=IQISCADCompleter::CandidateKind(
        index.sibling(index.row(), 0).data(IQISCADCompleter::KindRole).toInt() );

    auto c=textCursor();
    c.beginEditBlock();
    c.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, identifierPrefix().size());
    c.insertText(completion);

    bool isCommand =
        kind==IQISCADCompleter::Command
        || kind==IQISCADCompleter::Function
        || kind==IQISCADCompleter::PostprocCommand;
    if (isCommand && document()->characterAt(c.position())!=QChar('('))
    {
        c.insertText("(");
    }
    c.endEditBlock();
    setTextCursor(c);

    if (isCommand)
    {
        QString sig=commandSignature(completion);
        if (!sig.isEmpty())
        {
            QToolTip::showText(
                viewport()->mapToGlobal(cursorRect().bottomLeft()), sig, this );
        }
    }
}


void IQISCADModelScriptEdit::updateCalltip()
{
    QString cmd=enclosingCommand(textCursor().position());
    if (cmd!=calltipCommand_)
    {
        calltipCommand_=cmd;
        QString sig=commandSignature(cmd);
        if (!sig.isEmpty())
        {
            Q_EMIT displayStatusMessage(sig);
        }
    }
}


void IQISCADModelScriptEdit::keyPressEvent(QKeyEvent *e)
{
    auto *popup=completer_->popup();

    if (popup->isVisible())
    {
        // these keys are handled by the completer
        switch (e->key())
        {
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_Escape:
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
                e->ignore();
                return;
            default:
                break;
        }
    }

    if ( e->key()==Qt::Key_Space
         && (e->modifiers() & Qt::ControlModifier) )
    {
        if (e->modifiers() & Qt::ShiftModifier)
        {
            // show the signature of the current command
            QString sig=commandSignature(enclosingCommand(textCursor().position()));
            if (!sig.isEmpty())
            {
                QToolTip::showText(
                    viewport()->mapToGlobal(cursorRect().bottomLeft()), sig, this );
            }
        }
        else
        {
            showCompletion(true);
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(e);

    const QString t=e->text();
    if (t.isEmpty() || (e->modifiers() & (Qt::ControlModifier|Qt::AltModifier)))
        return;

    const int autoCompletionMinPrefix=3;
    const QString prefix=identifierPrefix();
    if (popup->isVisible())
    {
        if (prefix.isEmpty())
            popup->hide();
        else
            showCompletion(false);
    }
    else if (prefix.size()>=autoCompletionMinPrefix && isIdentifierChar(t.back()))
    {
        showCompletion(false);
    }

    if (t==QLatin1String("(") || t==QLatin1String(","))
    {
        QString sig=commandSignature(enclosingCommand(textCursor().position()));
        if (!sig.isEmpty())
        {
            QToolTip::showText(
                viewport()->mapToGlobal(cursorRect().bottomLeft()), sig, this );
        }
    }
}
