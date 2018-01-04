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



BGParsingThread::BGParsingThread()
: action_(ParseOnly)
{
}

void BGParsingThread::launch(const std::string& script, Action action)
{
    script_=script;
    action_=action;
    start();
}

void BGParsingThread::run()
{
//     std::istringstream is(script_);
// 
//     int failloc=-1;
// 
//     model_.reset(new insight::cad::Model);
// 
//     bool r=false;
//     try
//     {
//       r=insight::cad::parseISCADModelStream(is, model_.get(), &failloc, &syn_elem_dir_);
//     }
//     catch (...)
//     {
//     }
//     
//     if (!r) // fail if we did not get a full match
//     {
//         model_.reset();
//         syn_elem_dir_.reset();
//     }
    
    
    
    std::istringstream is(script_);

    int failloc=-1;

    insight::cad::cache.initRebuild();

    model_.reset(new insight::cad::Model);
    bool r=false;
    
    std::string reason="Failed: Syntax error";
    try
    {
        r=insight::cad::parseISCADModelStream(is, model_.get(), &failloc, &syn_elem_dir_);
    }
    catch (insight::cad::parser::iscadParserException e)
    {
        reason="Expected: "+e.message();
        failloc=e.from_pos();
        emit scriptError(failloc, reason);
    }

    if (!r) // fail if we did not get a full match
    {
        emit scriptError(failloc, "Syntax error");
    }
    else
    {

        emit statusMessage("Model parsed successfully.");
//             context_->getContext()->EraseAll();

        if (action_==ParseAndRebuild)
        {
            {
                auto scalars=model_->scalars();
                int is=0, ns=scalars.size();
                BOOST_FOREACH(decltype(scalars)::value_type const& v, scalars)
                {
                    emit statusMessage("Building scalar "+QString::fromStdString(v.first));
                    emit statusProgress(is++, ns);
                    emit addVariable(QString::fromStdString(v.first), v.second);
                }
            }

            {
                auto vectors=model_->vectors();
                int is=0, ns=vectors.size();
                BOOST_FOREACH(decltype(vectors)::value_type const& v, vectors)
                {
                    emit statusMessage("Building vector "+QString::fromStdString(v.first));
                    emit statusProgress(is++, ns);
                    emit addVariable(QString::fromStdString(v.first), v.second);
                }
            }

            {
                auto datums=model_->datums();
                int is=0, ns=datums.size();
                BOOST_FOREACH(decltype(datums)::value_type const& v, datums)
                {
                    emit statusMessage("Building datum "+QString::fromStdString(v.first));
                    emit statusProgress(is++, ns);
                    emit addDatum(QString::fromStdString(v.first), v.second);
                }
            }
            
            {
                auto modelsteps=model_->modelsteps();
                int is=0, ns=modelsteps.size();
                BOOST_FOREACH(decltype(modelsteps)::value_type const& v, modelsteps)
                {
                    bool is_comp=false;
                    if (model_->components().find(v.first) != model_->components().end())
                    {
                        is_comp=true;
                        emit statusMessage("Building component "+QString::fromStdString(v.first));
                    } else
                    {
                        emit statusMessage("Building feature "+QString::fromStdString(v.first));
                    }
                    emit statusProgress(is++, ns);
                    emit addFeature(QString::fromStdString(v.first), v.second, is_comp);
                }
            }

//             if (!skipPostprocActions_)
//             {
            {
                auto postprocActions=model_->postprocActions();
                int is=0, ns=postprocActions.size();
                BOOST_FOREACH(decltype(postprocActions)::value_type const& v, postprocActions)
                {
                    emit statusMessage("Building postproc action "+QString::fromStdString(v.first));
                    emit statusProgress(is++, ns);
                    emit addEvaluation(QString::fromStdString(v.first), v.second);
                }
            }
//             }

//             updateClipPlaneMenu();

            emit statusMessage("Model rebuild successfully finished.");
        }
        
        insight::cad::cache.finishRebuild();
    }
}



void ISCADModel::onGraphicalSelectionChanged(QoccViewWidget* aView)
{
    // Remove previously displayed sub objects from display
    BOOST_FOREACH(Handle_AIS_InteractiveObject& o, additionalDisplayObjectsForSelection_)
    {
        aView->getContext()->Erase(o, false);
    }
    additionalDisplayObjectsForSelection_.clear();

    // Display sub objects for current selection
    if (QFeatureItem* ms = checkGraphicalSelection<QFeatureItem>(aView))
    {
        insight::cad::Feature& sm=ms->solidmodel();
        const insight::cad::Feature::RefPointsList& pts=sm.getDatumPoints();

        // reverse storage to detect collocated points
        typedef std::map<arma::mat, std::string, insight::compareArmaMat> trpts;
        trpts rpts;
        BOOST_FOREACH(const insight::cad::Feature::RefPointsList::value_type& p, pts)
        {
            const std::string& name=p.first;
            const arma::mat& xyz=p.second;
//             std::cout<<name<<":"<<xyz<<std::endl;

            trpts::iterator j=rpts.find(xyz);
            if (j!=rpts.end())
            {
                j->second = j->second+"="+name;
            }
            else
            {
                rpts[xyz]=name;
            }
        }

        BOOST_FOREACH(const trpts::value_type& p, rpts)
        {
            const std::string& name=p.second;
            const arma::mat& xyz=p.first;
            Handle_AIS_InteractiveObject o
            (
                new insight::cad::InteractiveText(name, xyz)
            );
            additionalDisplayObjectsForSelection_.push_back(o);
            aView->getContext()->Display(o, false);
        }
    }

    aView->getContext()->UpdateCurrentViewer();
}



ISCADModel::ISCADModel(QWidget* parent, bool dobgparsing)
: QWidget(parent),
    unsaved_(false),
    doBgParsing_(dobgparsing),
    bgparsethread_(),
    skipPostprocActions_(true)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    QSplitter *spl=new QSplitter(Qt::Horizontal);
    layout->addWidget(spl);

    context_=new QoccViewerContext;

    viewer_=new QoccViewWidget(context_->getContext(), spl);
    spl->addWidget(viewer_);
    
    connect(viewer_,
            SIGNAL(popupMenu( QoccViewWidget*, const QPoint)),
            this,
            SLOT(popupMenu(QoccViewWidget*,const QPoint))
           );
    connect(viewer_,
            SIGNAL(selectionChanged(QoccViewWidget*)),
            this,
            SLOT(onGraphicalSelectionChanged(QoccViewWidget*))
           );

    editor_=new QTextEdit(spl);
    editor_->setFontFamily("Monospace");
    editor_->setContextMenuPolicy(Qt::CustomContextMenu);
    spl->addWidget(editor_);
    
    connect(editor_, SIGNAL(selectionChanged()), this, SLOT(onEditorSelectionChanged()));
    connect(editor_, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showEditorContextMenu(const QPoint&)));


    highlighter_=new ISCADSyntaxHighlighter(editor_->document());

    QSplitter* spl2=new QSplitter(Qt::Vertical, spl);
    QGroupBox *gb;
    QVBoxLayout *vbox;


    gb=new QGroupBox("Controls");
    vbox = new QVBoxLayout;
    QWidget*shw=new QWidget;
    QHBoxLayout *shbox = new QHBoxLayout;
    QPushButton *rebuildBtn=new QPushButton("Rebuild", gb);
    QPushButton *rebuildBtnUTC=new QPushButton("Rbld to Cursor", gb);
    connect(rebuildBtn, SIGNAL(clicked()), this, SLOT(rebuildModel()));
    connect(rebuildBtnUTC, SIGNAL(clicked()), this, SLOT(rebuildModelUpToCursor()));
    shbox->addWidget(rebuildBtn);
    shbox->addWidget(rebuildBtnUTC);
    shw->setLayout(shbox);
    vbox->addWidget(shw);
    
    QCheckBox *toggleBgParse=new QCheckBox("Do BG parsing", gb);
    toggleBgParse->setCheckState( doBgParsing_ ? Qt::Checked : Qt::Unchecked );
    connect(toggleBgParse, SIGNAL(stateChanged(int)), this, SLOT(toggleBgParsing(int)));
    vbox->addWidget(toggleBgParse);
    
    QCheckBox *toggleSkipPostprocActions=new QCheckBox("Skip Postproc Actions", gb);
    toggleSkipPostprocActions->setCheckState( skipPostprocActions_ ? Qt::Checked : Qt::Unchecked );
    connect(toggleSkipPostprocActions, SIGNAL(stateChanged(int)), this, SLOT(toggleSkipPostprocActions(int)));
    vbox->addWidget(toggleSkipPostprocActions);
    
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Model Tree");
    vbox = new QVBoxLayout;
    modeltree_=new QModelTree(gb);
    modeltree_->setMinimumHeight(20);
//     connect(modeltree_, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onModelTreeItemChanged(QTreeWidgetItem*, int)));
    vbox->addWidget(modeltree_);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Notepad");
    vbox = new QVBoxLayout;
    notepad_=new QTextEdit;

    vbox->addWidget(notepad_);
    QPushButton* copybtn=new QPushButton("<< Copy to cursor <<");
    vbox->addWidget(copybtn);
    connect(copybtn, SIGNAL(clicked()), this, SLOT(onCopyBtnClicked()));
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    spl->addWidget(spl2);
    
    QList<int> sizes;
    sizes << 500 << 350 << 150;
    spl->setSizes(sizes);
    
    connect(&bgparsethread_, SIGNAL(finished()), this, SLOT(onBgParseFinished()));
    bgparseTimer_=new QTimer(this);
    connect(bgparseTimer_, SIGNAL(timeout()), this, SLOT(doBgParse()));
    restartBgParseTimer();
    connect(editor_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(restartBgParseTimer(int,int,int)));
    connect(editor_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(setUnsavedState(int,int,int)));
   
}


ISCADModel::~ISCADModel()
{
//     bgparsethread_.stop();
    bgparsethread_.wait();
}


void ISCADModel::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::Yes;

    if (unsaved_)
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
                bool saved = saveModel();
                if (!saved)
                {
                    saved=saveModelAs();
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


bool ISCADModel::saveModel()
{
    if (filename_!="")
    {
        std::ofstream out(filename_.c_str());
        out << editor_->toPlainText().toStdString();
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
    context_->getContext()->EraseAll(
#if (OCC_VERSION_MAJOR>=7)
                   false
#endif                        
    );
    
    modeltree_->storeViewStates();
    modeltree_->clear();
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


    disconnect(editor_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(setUnsavedState(int,int,int)));
    editor_->setPlainText(contents_raw.c_str());
    connect(editor_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(setUnsavedState(int,int,int)));
}


void ISCADModel::setScript(const std::string& contents)
{
    clearDerivedData();

    disconnect(editor_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(setUnsavedState(int,int,int)));
    editor_->setPlainText(contents.c_str());
    connect(editor_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(setUnsavedState(int,int,int)));
}


void ISCADModel::onEditorSelectionChanged()
{
    QTextDocument *doc = editor_->document();
    QString word=editor_->textCursor().selectedText();
    if (!(word.contains('|')||word.contains('*')))
    {
        highlighter_->setHighlightWord(word);
        highlighter_->rehighlight();
    }
}




void ISCADModel::jump_to(const QString& name)
{
    highlighter_->setHighlightWord(name);
    highlighter_->rehighlight();

    QRegExp expression("\\b("+name+")\\b");
    int i=expression.indexIn(editor_->toPlainText());

//   qDebug()<<"jumping "<<name<<" at i="<<i<<endl;

    if (i>=0)
    {
        QTextCursor tmpCursor = editor_->textCursor();
        tmpCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1 );
        tmpCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, i );
        editor_->setTextCursor(tmpCursor);
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
            emit displayStatus("Background model parsing in progress...");
            bgparsethread_.launch( editor_->toPlainText().toStdString() );
        }
    }
}

void ISCADModel::onBgParseFinished()
{
    if (bgparsethread_.model_)
    {
        emit displayStatus("Background model parsing finished successfully.");
        cur_model_ = bgparsethread_.model_;
        syn_elem_dir_ = bgparsethread_.syn_elem_dir_;
        updateClipPlaneMenu();
    }
    else
    {
         emit displayStatus("Background model parsing failed.");
    }

}


void ISCADModel::allShaded()
{
    modeltree_->setUniformDisplayMode(AIS_Shaded);
}

void ISCADModel::allWireframe()
{
    modeltree_->setUniformDisplayMode(AIS_WireFrame);
}




void ISCADModel::populateClipPlaneMenu(QMenu* clipplanemenu)
{
    clipplanemenu->clear();
    
    if (cur_model_)
    {
        int added=0;
        auto datums=cur_model_->datums();
        BOOST_FOREACH(decltype(datums)::value_type const& v, datums)
        {
            insight::cad::DatumPtr d = v.second;
            if (d->providesPlanarReference())
            {
                QAction *act = new QAction( v.first.c_str(), this );
                
                QSignalMapper *signalMapper = new QSignalMapper(this);
                signalMapper->setMapping( act, reinterpret_cast<QObject*>(d.get()) );
                connect(signalMapper, SIGNAL(mapped(QObject*)),
                        this, SLOT(onSetClipPlane(QObject*)));
                connect(act, SIGNAL(triggered()), signalMapper, SLOT(map()));
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


void ISCADModel::onSetClipPlane(QObject* datumplane)
{
    insight::cad::Datum* datum = reinterpret_cast<insight::cad::Datum*>(datumplane);
    gp_Ax3 pl = datum->plane();
    gp_Pnt p = pl.Location();
    gp_Dir n = pl.Direction();
    viewer_->toggleClip( p.X(),p.Y(),p.Z(), n.X(),n.Y(),n.Z() );
}

void ISCADModel::onCopyBtnClicked()
{
  editor_->textCursor().insertText(notepad_->toPlainText());  
  notepad_->clear();
}


void ISCADModel::editSketch(QObject* sk_ptr)
{
    insight::cad::Sketch* sk = reinterpret_cast<insight::cad::Sketch*>(sk_ptr);
    sk->executeEditor();
}


void ISCADModel::editModel(QObject* sk_ptr)
{
    insight::cad::ModelFeature* sk = reinterpret_cast<insight::cad::ModelFeature*>(sk_ptr);
//     sk->executeEditor();
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
    editor_->textCursor().insertText("\n\n############################################################\n#####  ");
}

void ISCADModel::insertFeatureAtCursor()
{
    InsertFeatureDlg *dlg=new InsertFeatureDlg(this);
    if ( dlg->exec() == QDialog::Accepted )
    {
        editor_->textCursor().insertText(dlg->insert_string_);
    }
}


void ISCADModel::insertComponentNameAtCursor()
{
    ModelComponentSelectorDlg* dlg=new ModelComponentSelectorDlg(cur_model_, this);
    if ( dlg->exec() == QDialog::Accepted )
    {
        std::string id = dlg->selected();
        editor_->textCursor().insertText(id.c_str());
    }
}



void ISCADModel::onModelTreeItemChanged(QTreeWidgetItem * item, int)
{
    QDisplayableModelTreeItem* mi=dynamic_cast<QDisplayableModelTreeItem*>(item);
    if (mi)
    {
        mi->updateDisplay();
    }
}






void ISCADModel::addFeature(std::string sn, insight::cad::FeaturePtr sm, bool is_component)
{
    QFeatureItem* msi=modeltree_->addFeatureItem(sn, sm, context_, is_component);

    connect
    (
        msi, SIGNAL(insertParserStatementAtCursor(const QString&)),
        editor_, SLOT(insertPlainText(const QString&))
    );
    connect
    (
        msi, SIGNAL(jump_to(const QString&)),
        this, SLOT(jump_to(const QString&))
    );
    connect
    (
        msi, SIGNAL(setUniformDisplayMode(const AIS_DisplayMode)),
        modeltree_, SLOT(setUniformDisplayMode(const AIS_DisplayMode))
    );
    connect
    (
        msi, SIGNAL(addEvaluation(std::string, insight::cad::PostprocActionPtr, bool)),
        this, SLOT(addEvaluation(std::string, insight::cad::PostprocActionPtr, bool))
    );
}




void ISCADModel::addDatum(std::string sn, insight::cad::DatumPtr sm)
{
    modeltree_->addDatumItem(sn, sm, cur_model_, context_);
}




void ISCADModel::addEvaluation(std::string sn, insight::cad::PostprocActionPtr sm, bool visible)
{
    modeltree_->addEvaluationItem(sn, sm, context_);
}




void ISCADModel::addVariable(std::string sn, insight::cad::parser::scalar sv)
{
    modeltree_->addScalarVariableItem(sn, sv->value());
}




void ISCADModel::addVariable(std::string sn, insight::cad::parser::vector vv)
{
    QVectorVariableItem* msi = modeltree_->addVectorVariableItem(sn, vv->value(), context_);
    connect
    (
        msi, SIGNAL(insertParserStatementAtCursor(const QString&)),
        editor_, SLOT(insertPlainText(const QString&))
    );
}






void ISCADModel::rebuildModel(bool upToCursor)
{
    if (!bgparsethread_.isRunning())
    {
        disconnect(modeltree_, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onModelTreeItemChanged(QTreeWidgetItem*, int)));

        clearDerivedData();

        std::string script_content = editor_->toPlainText().toStdString();
        if (upToCursor)
        {
            QTextCursor c = editor_->textCursor();
            script_content = script_content.substr(0, c.position());
        }
        std::istringstream is(script_content);

        int failloc=-1;

        insight::cad::cache.initRebuild();

        cur_model_.reset(new insight::cad::Model);
        bool r=false;
        
        std::string reason="Failed: Syntax error";
        try
        {
            r=insight::cad::parseISCADModelStream(is, cur_model_.get(), &failloc);
        }
        catch (insight::cad::parser::iscadParserException e)
        {
            reason="Expected: "+e.message();
            failloc=e.from_pos();
        }

        if (!r) // fail if we did not get a full match
        {
            QTextCursor tmpCursor = editor_->textCursor();
            tmpCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1 );
            tmpCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, failloc );
            editor_->setTextCursor(tmpCursor);

            std::cout<<"Parser error at cursor location:"<<std::endl << reason <<std::endl;
            emit displayStatus(QString(reason.c_str())+" (Cursor moved to location where parsing stopped)!");
        }
        else
        {
            emit displayStatus("Model parsed successfully. Now performing rebuild...");

            context_->getContext()->EraseAll(
#if (OCC_VERSION_MAJOR>=7)
                   false
#endif                
            );

            auto modelsteps=cur_model_->modelsteps();
            BOOST_FOREACH(decltype(modelsteps)::value_type const& v, modelsteps)
            {
                bool is_comp=false;
                if (cur_model_->components().find(v.first)!=cur_model_->components().end())
                {
                    is_comp=true;
                }
                addFeature(v.first, v.second, is_comp);
            }

            auto datums=cur_model_->datums();
            BOOST_FOREACH(decltype(datums)::value_type const& v, datums)
            {
                addDatum(v.first, v.second);
            }

            if (!skipPostprocActions_)
            {
                auto postprocActions=cur_model_->postprocActions();
                BOOST_FOREACH(decltype(postprocActions)::value_type const& v, postprocActions)
                {
                    addEvaluation(v.first, v.second);
                }
            }

            auto scalars=cur_model_->scalars();
            BOOST_FOREACH(decltype(scalars)::value_type const& v, scalars)
            {
                addVariable(v.first, v.second);
            }

            auto vectors=cur_model_->vectors();
            BOOST_FOREACH(decltype(vectors)::value_type const& v, vectors)
            {
                addVariable(v.first, v.second);
            }
            
            updateClipPlaneMenu();

            emit displayStatus("Model rebuild successfully finished.");

            insight::cad::cache.finishRebuild();
        }
        
        connect(modeltree_, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onModelTreeItemChanged(QTreeWidgetItem*, int)));
    }
    else
    {
        emit displayStatus("Background model parsing in progress, rebuild is currently disabled!...");
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




void ISCADModel::popupMenu( QoccViewWidget* aView, const QPoint aPoint )
{
    if (aView->getContext()->HasDetected())
    {
        if (aView->getContext()->DetectedInteractive()->HasOwner())
        {
            Handle_Standard_Transient own=aView->getContext()->DetectedInteractive()->GetOwner();
            if (!own.IsNull())
            {
                if (PointerTransient *smo=dynamic_cast<PointerTransient*>(own
#if (OCC_VERSION_MAJOR<7)
                        .Access()
#else
                        .get()
#endif
                ))
                {
                    if (QModelTreeItem* mi=dynamic_cast<QModelTreeItem*>(smo->getPointer()))
                    {
                        // an item exists under the requested position
                        mi->showContextMenu(aView->mapToGlobal(aPoint));
                    }
                }
            }
        }
    }
}


void ISCADModel::showEditorContextMenu(const QPoint& pt)
{
    QMenu * menu = editor_->createStandardContextMenu();

    if (syn_elem_dir_)
    {
        QTextCursor cursor = editor_->cursorForPosition(pt);
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
                connect(act, SIGNAL(triggered()), signalMapper, SLOT(map()));
                menu->addSeparator();
                menu->addAction(act);
            }
        }
    }

    menu->exec(editor_->mapToGlobal(pt));
}




void ISCADModel::setUnsavedState(int, int rem, int ad)
{
//     std::cerr<<"CHANGED:"<<rem<<" "<<ad<<std::endl;
    if ((rem>0) || (ad>0))
    {
        if (!unsaved_)
        {
            emit updateTabTitle(this, filename_, true);
            unsaved_=true;
        }
    }
}



void ISCADModel::unsetUnsavedState()
{
    emit updateTabTitle(this, filename_, false);
    unsaved_=false;
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
//     editor_->setFontFamily("Monospace");
//     editor_->setContextMenuPolicy(Qt::CustomContextMenu);
    spl->addWidget(model_);

    connect(viewer_,
            SIGNAL(popupMenu( QoccViewWidget*, const QPoint)),
            this,
            SLOT(popupMenu(QoccViewWidget*,const QPoint))
           );
    connect(viewer_,
            SIGNAL(selectionChanged(QoccViewWidget*)),
            this,
            SLOT(onGraphicalSelectionChanged(QoccViewWidget*))
           );

    
    connect(model_, SIGNAL(selectionChanged()), this, SLOT(onEditorSelectionChanged()));
    connect(model_, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showEditorContextMenu(const QPoint&)));


//     highlighter_=new ISCADSyntaxHighlighter(model_->document());

    QSplitter* spl2=new QSplitter(Qt::Vertical, spl);
    QGroupBox *gb;
    QVBoxLayout *vbox;


    gb=new QGroupBox("Controls");
    vbox = new QVBoxLayout;
    QWidget*shw=new QWidget;
    QHBoxLayout *shbox = new QHBoxLayout;
    QPushButton *rebuildBtn=new QPushButton("Rebuild", gb);
    QPushButton *rebuildBtnUTC=new QPushButton("Rbld to Cursor", gb);
    connect(rebuildBtn, SIGNAL(clicked()), this, SLOT(rebuildModel()));
    connect(rebuildBtnUTC, SIGNAL(clicked()), this, SLOT(rebuildModelUpToCursor()));
    shbox->addWidget(rebuildBtn);
    shbox->addWidget(rebuildBtnUTC);
    shw->setLayout(shbox);
    vbox->addWidget(shw);
    
    QCheckBox *toggleBgParse=new QCheckBox("Do BG parsing", gb);
    toggleBgParse->setCheckState( doBgParsing_ ? Qt::Checked : Qt::Unchecked );
    connect(toggleBgParse, SIGNAL(stateChanged(int)), this, SLOT(toggleBgParsing(int)));
    vbox->addWidget(toggleBgParse);
    
    QCheckBox *toggleSkipPostprocActions=new QCheckBox("Skip Postproc Actions", gb);
    toggleSkipPostprocActions->setCheckState( skipPostprocActions_ ? Qt::Checked : Qt::Unchecked );
    connect(toggleSkipPostprocActions, SIGNAL(stateChanged(int)), this, SLOT(toggleSkipPostprocActions(int)));
    vbox->addWidget(toggleSkipPostprocActions);
    
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Model Tree");
    vbox = new QVBoxLayout;
    modeltree_=new QModelTree(gb);
    modeltree_->setMinimumHeight(20);
    connect(modeltree_, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(onModelTreeItemChanged(QTreeWidgetItem*, int)));
    vbox->addWidget(modeltree_);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Notepad");
    vbox = new QVBoxLayout;
    notepad_=new QTextEdit;

    vbox->addWidget(notepad_);
    QPushButton* copybtn=new QPushButton("<< Copy to cursor <<");
    vbox->addWidget(copybtn);
    connect(copybtn, SIGNAL(clicked()), this, SLOT(onCopyBtnClicked()));
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    spl->addWidget(spl2);
    
    QList<int> sizes;
    sizes << 500 << 350 << 150;
    spl->setSizes(sizes);
    
    connect(&bgparsethread_, SIGNAL(finished()), this, SLOT(onBgParseFinished()));
    bgparseTimer_=new QTimer(this);
    connect(bgparseTimer_, SIGNAL(timeout()), this, SLOT(doBgParse()));
    restartBgParseTimer();
    connect(model_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(restartBgParseTimer(int,int,int)));
    connect(model_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(setUnsavedState(int,int,int)));   
}
