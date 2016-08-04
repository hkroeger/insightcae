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
      std::cout<<name<<":"<<xyz<<std::endl;
      
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
      Handle_AIS_InteractiveObject o(new insight::cad::InteractiveText(name, xyz));
      additionalDisplayObjectsForSelection_.push_back(o);
      aView->getContext()->Display(o, false);
    }
  }
  
  aView->getContext()->UpdateCurrentViewer();
}




ISCADMainWindow::ISCADMainWindow(QWidget* parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags),
  unsaved_(false)
{  
  setWindowIcon(QIcon(":/resources/logo_insight_cae.png"));
  
  QSplitter *spl0=new QSplitter(Qt::Vertical);
  QSplitter *spl=new QSplitter(Qt::Horizontal);
  setCentralWidget(spl0);
  spl0->addWidget(spl);
  log_=new QTextEdit;
  logger_=new Q_DebugStream(std::cout, log_);
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
  
  gb=new QGroupBox("Variables");
  vbox = new QVBoxLayout;
  variablelist_=new QListWidget;
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
  act = new QAction(("Toggle &clip plane"), this);
  connect(act, SIGNAL(triggered()), viewer_, SLOT(toggleClip()));
  vmenu->addAction(act);
  act = new QAction(("Change background color..."), this);
  connect(act, SIGNAL(triggered()), viewer_, SLOT(background()));
  vmenu->addAction(act);

  QSettings settings;
  restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
  restoreState(settings.value("mainWindowState").toByteArray());
  
  bgparseTimer_=new QTimer(this);
  connect(bgparseTimer_, SIGNAL(timeout()), this, SLOT(doBgParse()));
  restartBgParseTimer();
  connect(editor_, SIGNAL(textChanged()), this, SLOT(restartBgParseTimer()));
  connect(editor_, SIGNAL(textChanged()), this, SLOT(setUnsavedState()));
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
                tr("The editor content is not saved.\nAre you sure?\n"),
                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                QMessageBox::No
            );
    }
    
    if (resBtn != QMessageBox::Yes) 
    {
        event->ignore();
    } 
    else 
    {
        QSettings settings;
        settings.setValue("mainWindowGeometry", saveGeometry());
        settings.setValue("mainWindowState", saveState());
        event->accept();
    }    
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




void ISCADMainWindow::saveModel()
{
  if (filename_!="")
  {
    std::ofstream out(filename_.c_str());
    out << editor_->toPlainText().toStdString();
    out.close();
    unsetUnsavedState();
  }
}




void ISCADMainWindow::saveModelAs()
{
  QString fn=QFileDialog::getSaveFileName(this, "Select location", "", "ISCAD Model Files (*.iscad)");
  if (fn!="")
  {
    setFilename(qPrintable(fn));
    saveModel();
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
  clearDerivedData();
  
  setFilename(file);
  std::ifstream in(file.c_str());
  
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());  
  
  disconnect(editor_, SIGNAL(textChanged()), this, SLOT(setUnsavedState()));
  editor_->setPlainText(contents_raw.c_str());
  connect(editor_, SIGNAL(textChanged()), this, SLOT(setUnsavedState()));
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
//   qDebug()<<"allWF"<<endl;
//     viewer_->getContext()->SetDisplayMode(AM, false);
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




void ISCADMainWindow::restartBgParseTimer()
{
  bgparseTimer_->setSingleShot(true);
  bgparseTimer_->start(bgparseInterval);
}



void ISCADMainWindow::doBgParse()
{
  syn_elem_dir_.reset();
  statusBar()->showMessage("Background model parsing in progress...");
  
  std::istringstream is(editor_->toPlainText().toStdString());
  
  int failloc=-1;
  
  insight::cad::ModelPtr m(new insight::cad::Model);
  
  bool r=insight::cad::parseISCADModelStream(is, m.get(), &failloc, &syn_elem_dir_);

  if (!r) // fail if we did not get a full match
  {
//     QTextCursor tmpCursor = editor_->textCursor();
//     tmpCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1 );
//     tmpCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, failloc );
//     editor_->setTextCursor(tmpCursor);
    
    statusBar()->showMessage("Background model parsing failed.");
  }
  else
  {
    statusBar()->showMessage("Background model parsing finished successfully.");
  }
}




void ISCADMainWindow::editSketch(int sk_ptr)
{
    insight::cad::Sketch* sk = reinterpret_cast<insight::cad::Sketch*>(sk_ptr);
    std::cout<<"Edit Sketch: "<<sk->fn().string()<<std::endl;
    sk->executeEditor();
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




void ISCADMainWindow::addModelStep(std::string sn, insight::cad::FeaturePtr sm, bool visible)
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
  
  QModelStepItem* msi=new QModelStepItem(sn, sm, context_, vd);
  
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
  
  insight::cad::ModelPtr m(new insight::cad::Model);
  bool r=insight::cad::parseISCADModelStream(is, m.get(), &failloc);

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

    auto modelsteps=m->modelsteps();
    BOOST_FOREACH(decltype(modelsteps)::value_type const& v, modelsteps)
    { 
      bool inivis=false;
      if (m->components().find(v.first)!=m->components().end())
	inivis=true;
      addModelStep(v.first, v.second, inivis); 
    }
    
    auto datums=m->datums();
    BOOST_FOREACH(decltype(datums)::value_type const& v, datums)
    { addDatum(v.first, v.second); }
    
    auto postprocActions=m->postprocActions();
    BOOST_FOREACH(decltype(postprocActions)::value_type const& v, postprocActions)
    { addEvaluation(v.first, v.second); }

    auto scalars=m->scalars();
    BOOST_FOREACH(decltype(scalars)::value_type const& v, scalars)
    { addVariable(v.first, v.second); }
    
    auto vectors=m->vectors();
    BOOST_FOREACH(decltype(vectors)::value_type const& v, vectors)
    { addVariable(v.first, v.second); }

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
        std::size_t po=editor_->textCursor().position();
        insight::cad::FeaturePtr fp=syn_elem_dir_->findElement(po);
        if (fp)
        {
            QSignalMapper *signalMapper = new QSignalMapper(this);
            QAction *act=NULL;
            if (insight::cad::Sketch* sk=dynamic_cast<insight::cad::Sketch*>(fp.get()))
            {
                act=new QAction("Edit Sketch...", this);
                signalMapper->setMapping(act, int(sk));
                connect(signalMapper, SIGNAL(mapped(int)), 
                        this, SLOT(editSketch(int)));
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




void ISCADMainWindow::setUnsavedState()
{
    if (!unsaved_)
    {
        setWindowTitle(QString("*") + filename_.filename().c_str());
        unsaved_=true;
    }
}



void ISCADMainWindow::unsetUnsavedState()
{
    setWindowTitle(filename_.filename().c_str());
    unsaved_=false;
}
