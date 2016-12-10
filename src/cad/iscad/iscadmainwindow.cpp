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

#include "iscadmainwindow.h"

#include "modelsteplist.h"
#include "datumlist.h"
#include "evaluationlist.h"
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

void ISCADMainWindow::onGraphicalSelectionChanged(QoccViewWidget* aView)
{
    // Remove previously displayed sub objects from display
    BOOST_FOREACH(Handle_AIS_InteractiveObject& o, additionalDisplayObjectsForSelection_)
    {
        aView->getContext()->Erase(o, false);
    }
    additionalDisplayObjectsForSelection_.clear();

    // Display sub objects for current selection
    if (QModelStepItem* ms=checkGraphicalSelection<QModelStepItem>(aView))
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
// 	      new AIS_LengthDimension(gp_Pnt(0,0,0), gp_Pnt(0.321,0,0), gp_Pln(gp_Pnt(0,0,0), gp_Vec(0,1,0)))
	    );
            additionalDisplayObjectsForSelection_.push_back(o);
            aView->getContext()->Display(o, false);
        }
    }

    aView->getContext()->UpdateCurrentViewer();
}




ISCADMainWindow::ISCADMainWindow(QWidget* parent, Qt::WindowFlags flags, bool nolog)
: QMainWindow(parent, flags),
    unsaved_(false),
    doBgParsing_(true),
    bgparsethread_(),
    skipPostprocActions_(true)
{
    connect(&bgparsethread_, SIGNAL(finished()), this, SLOT(onBgParseFinished()));
    
    setWindowIcon(QIcon(":/resources/logo_insight_cae.png"));

    QSplitter *spl0=new QSplitter(Qt::Vertical);
    QSplitter *spl=new QSplitter(Qt::Horizontal);
    setCentralWidget(spl0);
    spl0->addWidget(spl);
    log_=new QTextEdit;

    if (!nolog)
    {
      logger_=new Q_DebugStream(std::cout); // ceases to work with multithreaded bg parsing
      connect(logger_, SIGNAL(appendText(const QString&)), log_, SLOT(append(const QString&)));
    }
    
    spl0->addWidget(log_);
    context_=new QoccViewerContext;

    viewer_=new QoccViewWidget(context_->getContext(), spl);
    connect(viewer_,
            SIGNAL(popupMenu( QoccViewWidget*, const QPoint)),
            this,
            SLOT(popupMenu(QoccViewWidget*,const QPoint))
           );
    spl->addWidget(viewer_);

    connect(viewer_,
            SIGNAL(selectionChanged(QoccViewWidget*)),
            this,
            SLOT(onGraphicalSelectionChanged(QoccViewWidget*))
           );

    editor_=new QTextEdit(spl);
    editor_->setFontFamily("Monospace");
    spl->addWidget(editor_);
    connect(editor_, SIGNAL(selectionChanged()), this, SLOT(onEditorSelectionChanged()));
    editor_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(editor_, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(showEditorContextMenu(const QPoint&)));


    highlighter_=new ISCADSyntaxHighlighter(editor_->document());


    QSplitter* spl2=new QSplitter(Qt::Vertical, spl);
    QGroupBox *gb;
    QVBoxLayout *vbox;


    gb=new QGroupBox("Controls");
    vbox = new QVBoxLayout;
    QPushButton *rebuildBtn=new QPushButton("Rebuild", gb);
    connect(rebuildBtn, SIGNAL(clicked()), this, SLOT(rebuildModel()));
    vbox->addWidget(rebuildBtn);
    
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

    gb=new QGroupBox("Variables");
    vbox = new QVBoxLayout;
    variablelist_=new QListWidget;
    variablelist_->setMinimumHeight(20);
    connect(variablelist_, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onVariableItemChanged(QListWidgetItem*)));
    vbox->addWidget(variablelist_);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Model steps");
    vbox = new QVBoxLayout;
    modelsteplist_=new ModelStepList;
    connect(modelsteplist_, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onModelStepItemChanged(QListWidgetItem*)));
    vbox->addWidget(modelsteplist_);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Datums");
    vbox = new QVBoxLayout;
    datumlist_=new DatumList;
    connect(datumlist_, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onDatumItemChanged(QListWidgetItem*)));
    vbox->addWidget(datumlist_);
    gb->setLayout(vbox);
    spl2->addWidget(gb);

    gb=new QGroupBox("Evaluation reports");
    vbox = new QVBoxLayout;
    evaluationlist_=new EvaluationList;
    connect(evaluationlist_, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onEvaluationItemChanged(QListWidgetItem*)));
    vbox->addWidget(evaluationlist_);
    gb->setLayout(vbox);
    spl2->addWidget(gb);
    
    gb=new QGroupBox("Notepad");
    vbox = new QVBoxLayout;
    notepad_=new QTextEdit;
//     connect(evaluationlist_, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onEvaluationItemChanged(QListWidgetItem*)));
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

    QMenu *fmenu = menuBar()->addMenu("&File");
    QMenu *mmenu = menuBar()->addMenu("&Model");
    QMenu *vmenu = menuBar()->addMenu("&View");

    QAction* act;
    act = new QAction(("&Load"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(loadModel()));
    fmenu->addAction(act);

    act = new QAction(("&Save"), this);
    act->setShortcut(Qt::ControlModifier + Qt::Key_S);
    connect(act, SIGNAL(triggered()), this, SLOT(saveModel()));
    fmenu->addAction(act);

    act = new QAction(("&Save as..."), this);
    connect(act, SIGNAL(triggered()), this, SLOT(saveModelAs()));
    fmenu->addAction(act);

    act = new QAction(("&Rebuild model"), this);
    act->setShortcut(Qt::ControlModifier + Qt::Key_Return);
    connect(act, SIGNAL(triggered()), this, SLOT(rebuildModel()));
    mmenu->addAction(act);

    act = new QAction(("Insert &feature..."), this);
    act->setShortcut(Qt::ControlModifier + Qt::Key_F);
    connect(act, SIGNAL(triggered()), this, SLOT(insertFeatureAtCursor()));
    mmenu->addAction(act);

    act = new QAction(("Insert &component name..."), this);
    act->setShortcut(Qt::ControlModifier + Qt::Key_I);
    connect(act, SIGNAL(triggered()), this, SLOT(insertComponentNameAtCursor()));
    mmenu->addAction(act);

    act = new QAction(("C&lear cache"), this);
    act->setShortcut(Qt::ControlModifier + Qt::Key_L);
    connect(act, SIGNAL(triggered()), this, SLOT(clearCache()));
    mmenu->addAction(act);

    act = new QAction(("Fit &all"), this);
    connect(act, SIGNAL(triggered()), viewer_, SLOT(fitAll()));
    act->setShortcut(Qt::ControlModifier + Qt::Key_A);
    vmenu->addAction(act);
    act = new QAction(("Toggle &grid"), this);
    connect(act, SIGNAL(triggered()), context_, SLOT(toggleGrid()));
    vmenu->addAction(act);
    
    act = new QAction(("Toggle clip plane (&XY)"), this);
    connect(act, SIGNAL(triggered()), viewer_, SLOT(toggleClipXY()));
    vmenu->addAction(act);
    act = new QAction(("Toggle clip plane (&YZ)"), this);
    connect(act, SIGNAL(triggered()), viewer_, SLOT(toggleClipYZ()));
    vmenu->addAction(act);
    act = new QAction(("Toggle clip plane (X&Z)"), this);
    connect(act, SIGNAL(triggered()), viewer_, SLOT(toggleClipXZ()));
    vmenu->addAction(act);
    
    clipplanemenu_=vmenu->addMenu("Clip at datum plane");
    clipplanemenu_->setDisabled(true);
    
    act = new QAction(("Change background color..."), this);
    connect(act, SIGNAL(triggered()), viewer_, SLOT(background()));
    vmenu->addAction(act);
    act = new QAction(("Display all &shaded"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(allShaded()));
    vmenu->addAction(act);
    act = new QAction(("Display all &wireframe"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(allWireframe()));
    vmenu->addAction(act);
    act = new QAction(("&Reset shading and visibility"), this);
    connect(act, SIGNAL(triggered()), this, SLOT(resetViz()));
//     act->setShortcut(Qt::ControlModifier + Qt::Key_A);
    vmenu->addAction(act);

    QSettings settings;
    restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
    restoreState(settings.value("mainWindowState").toByteArray());

    bgparseTimer_=new QTimer(this);
    connect(bgparseTimer_, SIGNAL(timeout()), this, SLOT(doBgParse()));
    restartBgParseTimer();
    connect(editor_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(restartBgParseTimer(int,int,int)));
    connect(editor_->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(setUnsavedState(int,int,int)));
    
}

ISCADMainWindow::~ISCADMainWindow()
{
//     bgparsethread_.stop();
    bgparsethread_.wait();
}


void ISCADMainWindow::closeEvent(QCloseEvent *event)
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
    
    QSettings settings;
    settings.setValue("mainWindowGeometry", saveGeometry());
    settings.setValue("mainWindowState", saveState());
    event->accept();

}




void ISCADMainWindow::loadModel()
{
    QString fn=QFileDialog::getOpenFileName(this, "Select file", "", "ISCAD Model Files (*.iscad)");
    if (fn!="")
    {
        loadFile(qPrintable(fn));
        unsetUnsavedState();
    }
}




bool ISCADMainWindow::saveModel()
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




bool ISCADMainWindow::saveModelAs()
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




void ISCADMainWindow::clearDerivedData()
{
    context_->getContext()->EraseAll();
    modelsteplist_->clear();
    datumlist_->clear();
    variablelist_->clear();
    evaluationlist_->clear();
}




void ISCADMainWindow::loadFile(const boost::filesystem::path& file)
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




void ISCADMainWindow::onEditorSelectionChanged()
{
    QTextDocument *doc = editor_->document();
    QString word=editor_->textCursor().selectedText();
    if (!(word.contains('|')||word.contains('*')))
    {
        highlighter_->setHighlightWord(word);
        highlighter_->rehighlight();
    }
}




void ISCADMainWindow::jump_to(const QString& name)
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




void ISCADMainWindow::setUniformDisplayMode(const AIS_DisplayMode AM)
{
    for (int i=0; i<modelsteplist_->count(); i++)
    {
        if (QModelStepItem *msi =dynamic_cast<QModelStepItem*>(modelsteplist_->item(i)))
        {
            if (AM==AIS_WireFrame)
                msi->wireframe();
            else if (AM==AIS_Shaded)
                msi->shaded();
        }
    }
}




void ISCADMainWindow::restartBgParseTimer(int,int,int)
{
    bgparseTimer_->setSingleShot(true);
    bgparseTimer_->start(bgparseInterval);
}


BGParsingThread::BGParsingThread()
{
}

void BGParsingThread::launch(const std::string& script)
{
    script_=script;
    start();
}

void BGParsingThread::run()
{
    std::istringstream is(script_);

    int failloc=-1;

    model_.reset(new insight::cad::Model);

    bool r=insight::cad::parseISCADModelStream(is, model_.get(), &failloc, &syn_elem_dir_);
    
    if (!r) // fail if we did not get a full match
    {
        model_.reset();
        syn_elem_dir_.reset();
    }
}
 
void ISCADMainWindow::doBgParse()
{
    if (doBgParsing_)
    {
        if (!bgparsethread_.isRunning())
        {
            statusBar()->showMessage("Background model parsing in progress...");
            bgparsethread_.launch( editor_->toPlainText().toStdString() );
        }
    }
}

void ISCADMainWindow::onBgParseFinished()
{
    if (bgparsethread_.model_)
    {
        statusBar()->showMessage("Background model parsing finished successfully.");
        cur_model_ = bgparsethread_.model_;
        syn_elem_dir_ = bgparsethread_.syn_elem_dir_;
        updateClipPlaneMenu();
    }
    else
    {
         statusBar()->showMessage("Background model parsing failed.");
    }

}


void ISCADMainWindow::allShaded()
{
    setUniformDisplayMode(AIS_Shaded);
}

void ISCADMainWindow::allWireframe()
{
    setUniformDisplayMode(AIS_WireFrame);
}

void ISCADMainWindow::resetViz()
{
    for (int i=0; i<modelsteplist_->count(); i++)
    {
        if ( QModelStepItem *qmsi=dynamic_cast<QModelStepItem*>(modelsteplist_->item(i)) )
        {
            qmsi->resetDisplay();
            qmsi->shaded();
        }
    }
}


void ISCADMainWindow::updateClipPlaneMenu()
{
    clipplanemenu_->clear();
    
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
            clipplanemenu_->addAction(act);
            
            added++;
        }
    }
    
    if (added)
    {
        clipplanemenu_->setEnabled(true);
    }
    else
    {
        clipplanemenu_->setDisabled(true);
    }
}


void ISCADMainWindow::onSetClipPlane(QObject* datumplane)
{
    insight::cad::Datum* datum = reinterpret_cast<insight::cad::Datum*>(datumplane);
    gp_Ax3 pl = datum->plane();
    gp_Pnt p = pl.Location();
    gp_Dir n = pl.Direction();
    viewer_->toggleClip( p.X(),p.Y(),p.Z(), n.X(),n.Y(),n.Z() );
}

void ISCADMainWindow::onCopyBtnClicked()
{
  editor_->textCursor().insertText(notepad_->toPlainText());  
  notepad_->clear();
}



void ISCADMainWindow::editSketch(QObject* sk_ptr)
{
    insight::cad::Sketch* sk = reinterpret_cast<insight::cad::Sketch*>(sk_ptr);
    sk->executeEditor();
}


void ISCADMainWindow::editModel(QObject* sk_ptr)
{
    insight::cad::ModelFeature* sk = reinterpret_cast<insight::cad::ModelFeature*>(sk_ptr);
    sk->executeEditor();
}




void ISCADMainWindow::toggleBgParsing(int state)
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


void ISCADMainWindow::toggleSkipPostprocActions(int state)
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


void ISCADMainWindow::insertFeatureAtCursor()
{
    InsertFeatureDlg *dlg=new InsertFeatureDlg(this);
    if ( dlg->exec() == QDialog::Accepted )
    {
        editor_->textCursor().insertText(dlg->insert_string_);
    }
}


void ISCADMainWindow::insertComponentNameAtCursor()
{
    ModelComponentSelectorDlg* dlg=new ModelComponentSelectorDlg(cur_model_, this);
    if ( dlg->exec() == QDialog::Accepted )
    {
        std::string id = dlg->selected();
        editor_->textCursor().insertText(id.c_str());
    }
}



void ISCADMainWindow::onVariableItemChanged(QListWidgetItem * item)
{
    QVariableItem* mi=dynamic_cast<QVariableItem*>(item);
    if (mi)
    {
        mi->updateDisplay();
    }
}




void ISCADMainWindow::onModelStepItemChanged(QListWidgetItem * item)
{
    QModelStepItem* mi=dynamic_cast<QModelStepItem*>(item);
    if (mi)
    {
        mi->updateDisplay();
    }
}




void ISCADMainWindow::onDatumItemChanged(QListWidgetItem * item)
{
    QDatumItem* mi=dynamic_cast<QDatumItem*>(item);
    if (mi)
    {
        mi->updateDisplay();
    }
}




void ISCADMainWindow::onEvaluationItemChanged(QListWidgetItem * item)
{
    QEvaluationItem* mi=dynamic_cast<QEvaluationItem*>(item);
    if (mi)
    {
        mi->updateDisplay();
    }
}




void ISCADMainWindow::addModelStep(std::string sn, insight::cad::FeaturePtr sm, bool visible, bool is_component)
{
    ViewState vd;

    if (visible)
        vd.visible=true;
    else
        vd.visible=false;

    if (checked_modelsteps_.find(sn)!=checked_modelsteps_.end())
    {
        vd=checked_modelsteps_.find(sn)->second;
    }

    QModelStepItem* msi=new QModelStepItem(sn, sm, context_, vd, 0, is_component);

//   ModelStepItemAdder* ia
//    = new ModelStepItemAdder(this, msi);
//   connect(ia, SIGNAL(finished()),
// 	  ia, SLOT(deleteLater()));
//   ia->start();

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
        this, SLOT(setUniformDisplayMode(const AIS_DisplayMode))
    );
    connect
    (
        msi, SIGNAL(addEvaluation(std::string, insight::cad::PostprocActionPtr, bool)),
        this, SLOT(addEvaluation(std::string, insight::cad::PostprocActionPtr, bool))
    );
    modelsteplist_->addItem(msi);
}




void ISCADMainWindow::addDatum(std::string sn, insight::cad::DatumPtr sm)
{
    ViewState vd;
    vd.visible=false;
//   if (sm->isleaf()) vd.visible=true; else vd.visible=false;

    if (checked_datums_.find(sn)!=checked_datums_.end())
    {
        vd=checked_datums_.find(sn)->second;
    }

    datumlist_->addItem(new QDatumItem(sn, sm, context_, vd));
}




void ISCADMainWindow::addEvaluation(std::string sn, insight::cad::PostprocActionPtr sm, bool visible)
{
    ViewState vd(0);
    vd.visible=visible;
//   if (sm->isleaf()) vd.visible=true; else vd.visible=false;

    if (checked_evaluations_.find(sn)!=checked_evaluations_.end())
    {
        vd=checked_evaluations_.find(sn)->second;
    }

    evaluationlist_->addItem(new QEvaluationItem(sn, sm, context_, vd));
}




void ISCADMainWindow::addVariable(std::string sn, insight::cad::parser::scalar sv)
{
    variablelist_->addItem
    (
        new QListWidgetItem
        (
            QString::fromStdString(sn+" = "+boost::lexical_cast<std::string>(sv->value()))
        )
    );
}




void ISCADMainWindow::addVariable(std::string sn, insight::cad::parser::vector vv)
{
    ViewState vd;
    vd.visible=false;

    QVariableItem* msi=new QVariableItem(sn, vv->value(), context_, vd);
    connect
    (
        msi, SIGNAL(insertParserStatementAtCursor(const QString&)),
        editor_, SLOT(insertPlainText(const QString&))
    );

    variablelist_->addItem(msi);
}






void ISCADMainWindow::rebuildModel()
{
    log_->clear();

    //checked_modelsteps_.clear();
    for (int i=0; i<modelsteplist_->count(); i++)
    {
        QModelStepItem *qmsi=dynamic_cast<QModelStepItem*>(modelsteplist_->item(i));
        if (qmsi)
        {
            checked_modelsteps_[qmsi->text().toStdString()]=qmsi->state_;
        }
    }

    for (int i=0; i<datumlist_->count(); i++)
    {
        QDatumItem *qmsi=dynamic_cast<QDatumItem*>(datumlist_->item(i));
        if (qmsi)
        {
            checked_datums_[qmsi->text().toStdString()]=qmsi->state_;
        }
    }

    for (int i=0; i<evaluationlist_->count(); i++)
    {
        QEvaluationItem *qmsi=dynamic_cast<QEvaluationItem*>(evaluationlist_->item(i));
        if (qmsi)
        {
            checked_evaluations_[qmsi->text().toStdString()]=qmsi->state_;
        }
    }

    clearDerivedData();

    std::istringstream is(editor_->toPlainText().toStdString());

    int failloc=-1;

    insight::cad::cache.initRebuild();

    cur_model_.reset(new insight::cad::Model);
    bool r=insight::cad::parseISCADModelStream(is, cur_model_.get(), &failloc);

    if (!r) // fail if we did not get a full match
    {
        QTextCursor tmpCursor = editor_->textCursor();
        tmpCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1 );
        tmpCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, failloc );
        editor_->setTextCursor(tmpCursor);

        statusBar()->showMessage("Model parsing failed => Cursor moved to location where parsing stopped!");
    }
    else
    {
        statusBar()->showMessage("Model parsed successfully. Now performing rebuild...");

        context_->getContext()->EraseAll();

        auto modelsteps=cur_model_->modelsteps();
        BOOST_FOREACH(decltype(modelsteps)::value_type const& v, modelsteps)
        {
            bool is_comp=false;
            if (cur_model_->components().find(v.first)!=cur_model_->components().end())
            {
                is_comp=true;
            }
            addModelStep(v.first, v.second, is_comp, is_comp);
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

        statusBar()->showMessage("Model rebuild successfully finished.");

        insight::cad::cache.finishRebuild();
    }
}




void ISCADMainWindow::clearCache()
{
    insight::cad::cache.clear();
}




void ISCADMainWindow::popupMenu( QoccViewWidget* aView, const QPoint aPoint )
{
    if (aView->getContext()->HasDetected())
    {
        if (aView->getContext()->DetectedInteractive()->HasOwner())
        {
            Handle_Standard_Transient own=aView->getContext()->DetectedInteractive()->GetOwner();
            if (!own.IsNull())
            {
                if (PointerTransient *smo=dynamic_cast<PointerTransient*>(own.Access()))
                {
                    if (QModelStepItem* mi=dynamic_cast<QModelStepItem*>(smo->getPointer()))
                    {
                        // an item exists under the requested position
                        mi->showContextMenu(aView->mapToGlobal(aPoint));
                    }
                    else if (QVariableItem* mi=dynamic_cast<QVariableItem*>(smo->getPointer()))
                    {
                        // an item exists under the requested position
                        mi->showContextMenu(aView->mapToGlobal(aPoint));
                    }
                }
            }
        }
    }
}


void ISCADMainWindow::showEditorContextMenu(const QPoint& pt)
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




void ISCADMainWindow::setUnsavedState(int, int rem, int ad)
{
//     std::cerr<<"CHANGED:"<<rem<<" "<<ad<<std::endl;
    if ((rem>0) || (ad>0))
    {
        if (!unsaved_)
        {
            setWindowTitle(QString("*") + filename_.filename().c_str());
            unsaved_=true;
        }
    }
}



void ISCADMainWindow::unsetUnsavedState()
{
    setWindowTitle(filename_.filename().c_str());
    unsaved_=false;
}
