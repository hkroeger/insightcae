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

#include "occinclude.h"
#include "AIS.hxx"
#include "AIS_Shape.hxx"
#include "AIS_InteractiveContext.hxx"

#include "solidmodel.h"
#include "solidmodeltransient.h"
#include "datum.h"
#include "evaluation.h"

#include "qoccviewwidget.h"
#include "qoccviewercontext.h"

#include <QMessageBox>
#include <QMainWindow>

#include "iscadapplication.h"
#include "base/exception.h"
#include "base/linearalgebra.h"

#include <iostream>

using namespace std;
using namespace boost;
using namespace insight;
using namespace insight::cad;
using namespace insight::cad::parser;

ISCADApplication::ISCADApplication( int &argc, char **argv)
: QApplication(argc, argv)
{}

ISCADApplication::~ISCADApplication( )
{}

bool ISCADApplication::notify(QObject *rec, QEvent *ev)
{
  try
  {
    return QApplication::notify(rec, ev);
  }
  catch (insight::Exception e)
  {
    std::cout << e << std::endl;
    
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(QString(e.as_string().c_str()));
/*    if (e.addInfo()!="")
    {
      msgBox.setInformativeText("Please check additional info.");
      msgBox.setDetailedText(QString(e.addInfo().c_str()));
    }*/
    msgBox.exec();
//    QMessageBox::critical
//    (
//        activeWindow(), "Error",
//        QString(("An error occured in PropGeo:\n"+e.message()).c_str())
//    );
  }
  /*
  catch (Standard_Failure e)
  {
    QMessageBox::critical
    (
	activeWindow(), "Error",
	QString("An error occured in OpenCASCADE:\n")+e.GetMessageString()
    );
  }*/

  return true;
}

ModelStepList::ModelStepList(QWidget* parent)
: QListWidget(parent)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect
  (
    this,
    SIGNAL(customContextMenuRequested(const QPoint &)),
    this,
    SLOT(showContextMenuForWidget(const QPoint &))
  );
}

DatumList::DatumList(QWidget* parent)
: QListWidget(parent)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect
  (
    this,
    SIGNAL(customContextMenuRequested(const QPoint &)),
    this,
    SLOT(showContextMenuForWidget(const QPoint &))
  );
}

EvaluationList::EvaluationList(QWidget* parent)
: QListWidget(parent)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect
  (
    this,
    SIGNAL(customContextMenuRequested(const QPoint &)),
    this,
    SLOT(showContextMenuForWidget(const QPoint &))
  );
}

ISCADMainWindow::ISCADMainWindow(QWidget* parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags)
{  
  QSplitter *spl=new QSplitter(Qt::Horizontal);
  setCentralWidget(spl);
  context_=new QoccViewerContext;
  
  viewer_=new QoccViewWidget(context_->getContext(), spl);
  connect(viewer_, 
	  SIGNAL(popupMenu( const QoccViewWidget*, const QPoint)),
	  this,
	  SLOT(popupMenu(const QoccViewWidget*,const QPoint))
 	);
  spl->addWidget(viewer_);
  
  editor_=new QTextEdit(spl);
  spl->addWidget(editor_);
  
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
  
  act = new QAction(("Fit &all"), this);
  connect(act, SIGNAL(triggered()), viewer_, SLOT(fitAll()));
  act->setShortcut(Qt::ControlModifier + Qt::Key_A);
  vmenu->addAction(act);
  act = new QAction(("Toggle &grid"), this);
  connect(act, SIGNAL(triggered()), context_, SLOT(toggleGrid()));
  vmenu->addAction(act);

  QSettings settings;
  restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
  restoreState(settings.value("mainWindowState").toByteArray());
}

void ISCADMainWindow::closeEvent(QCloseEvent *event) 
{
  QSettings settings;
  settings.setValue("mainWindowGeometry", saveGeometry());
  settings.setValue("mainWindowState", saveState());
}

void ISCADMainWindow::loadModel()
{
  QString fn=QFileDialog::getOpenFileName(this, "Select file", "", "ISCAD Model Files (*.iscad)");
  if (fn!="")
    loadFile(qPrintable(fn));
}

void ISCADMainWindow::saveModel()
{
  if (filename_!="")
  {
    std::ofstream out(filename_.c_str());
    out << editor_->toPlainText().toStdString();
    out.close();
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
  
  editor_->setPlainText(contents_raw.c_str());
}

/**
 * Needed since QMainWindows copy constructor is not available
 */
struct Transferrer
{
  ISCADMainWindow& mw_;
  
  Transferrer(ISCADMainWindow& mw)
  : mw_(mw)
  {
  }
  
  void operator()(std::string sn, insight::cad::SolidModelPtr sm)
  {
    cout<<sn<<" : "<<sm.get()<<endl;
    mw_.addModelStep(sn, sm);
  }

  void operator()(std::string sn, insight::cad::parser::scalar sv)
  {
    mw_.addVariable(sn, sv);
  }

  void operator()(std::string sn, insight::cad::parser::vector vv)
  {
    mw_.addVariable(sn, vv);
  }
};


ViewState::ViewState()
: shading(1),
  visible(true),
  r(0.5),
  g(0.5),
  b(0.5)
{
  randomizeColor();
}

void ViewState::randomizeColor()
{
  r=0.5+0.5*( double(rand()) / double(RAND_MAX) );
  g=0.5+0.5*( double(rand()) / double(RAND_MAX) );
  b=0.5+0.5*( double(rand()) / double(RAND_MAX) );
}

class PointerTransient 
: public Standard_Transient
{
protected:
  QObject* mi_;
  
public:
    PointerTransient();
    PointerTransient(const PointerTransient& o);
    PointerTransient(QObject* mi);
    ~PointerTransient();
    void operator=(QObject* mi);
    QObject* getPointer();
    
};



QModelStepItem::QModelStepItem(const std::string& name, SolidModelPtr smp, QoccViewerContext* context, 
		const ViewState& state, QListWidget* view )
: QListWidgetItem(QString::fromStdString(name), view),
  name_(QString::fromStdString(name)),
  context_(context),
  state_(state)
{
  setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
  reset(smp);
}

void QModelStepItem::reset(SolidModelPtr smp)
{
  smp_=smp;
  if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
  ais_=new AIS_Shape(*smp_);
//     Handle_Standard_Transient owner_container(new SolidModelTransient(smp));
  Handle_Standard_Transient owner_container(new PointerTransient(this));
  ais_->SetOwner(owner_container);
  context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
  updateDisplay();
}

void QModelStepItem::wireframe()
{
  state_.shading=0;
  updateDisplay();
}

void QModelStepItem::shaded()
{
  state_.shading=1;
  updateDisplay();
}

void QModelStepItem::randomizeColor()
{
  state_.randomizeColor();
  updateDisplay();
}

void QModelStepItem::updateDisplay()
{
  state_.visible = (checkState()==Qt::Checked);
  
  if (state_.visible)
  {
    context_->getContext()->Display(ais_);
    context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
    context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
  }
  else
  {
    context_->getContext()->Erase(ais_);
  }
}

void QModelStepItem::exportShape()
{
  QString fn=QFileDialog::getSaveFileName
  (
    listWidget(), 
    "Export file name", 
    "", "BREP file (*,brep);;ASCII STL file (*.stl);;Binary STL file (*.stlb);;IGES file (*.igs);;STEP file (*.stp)"
  );
  if (!fn.isEmpty()) smp_->saveAs(qPrintable(fn));
}

void QModelStepItem::insertName()
{
  emit insertParserStatementAtCursor(name_);
}

void QModelStepItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    // for QAbstractScrollArea and derived classes you would use:
    // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

    QMenu myMenu;
    QAction *tit=new QAction(name_, &myMenu);
    tit->setDisabled(true);
    myMenu.addAction(tit);
    myMenu.addSeparator();
    myMenu.addAction("Shaded");
    myMenu.addAction("Wireframe");
    myMenu.addAction("Randomize Color");
    myMenu.addAction("Insert name");
    myMenu.addAction("Export...");
    // ...

    QAction* selectedItem = myMenu.exec(gpos);
    if (selectedItem)
    {
	if (selectedItem->text()=="Shaded") shaded();
	if (selectedItem->text()=="Wireframe") wireframe();
	if (selectedItem->text()=="Randomize Color") randomizeColor();
	if (selectedItem->text()=="Insert name") insertName();
	if (selectedItem->text()=="Export...") exportShape();
    }
    else
    {
	// nothing was chosen
    }
}

PointerTransient::PointerTransient()
: mi_(NULL)
{}

PointerTransient::PointerTransient(const PointerTransient& o)
: mi_(o.mi_)
{}

PointerTransient::PointerTransient(QObject* mi)
: mi_(mi)
{}

PointerTransient::~PointerTransient()
{}

void PointerTransient::operator=(QObject* mi)
{
  mi_=mi;
}

QObject *PointerTransient::getPointer()
{
  return mi_;
}


void ModelStepList::showContextMenuForWidget(const QPoint &p)
{
  QModelStepItem * mi=dynamic_cast<QModelStepItem*>(itemAt(p));
  if (mi)
  {
    mi->showContextMenu(this->mapToGlobal(p));
  }
}

class QDatumItem
: public QListWidgetItem
{
  DatumPtr smp_;
  QoccViewerContext* context_;
  Handle_AIS_InteractiveObject ais_;
    
public:
  ViewState state_;

  QDatumItem(const std::string& name, DatumPtr smp, QoccViewerContext* context, 
		 const ViewState& state, QListWidget* view = 0)
  : QListWidgetItem(QString::fromStdString(name), view),
    context_(context),
    state_(state)
  {
    setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
    reset(smp);
  }
  
  void reset(DatumPtr smp)
  {
    smp_=smp;
    if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
    ais_=smp_->createAISRepr();
    context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
    updateDisplay();
  }
  
  void wireframe()
  {
    state_.shading=0;
    updateDisplay();
  }

  void shaded()
  {
    state_.shading=1;
    updateDisplay();
  }
  
  void randomizeColor()
  {
    state_.randomizeColor();
    updateDisplay();
  }
  
  void updateDisplay()
  {
    state_.visible = (checkState()==Qt::Checked);
    
    if (state_.visible)
    {
      context_->getContext()->Display(ais_);
      context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
      context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
    }
    else
    {
      context_->getContext()->Erase(ais_);
    }
  }
  
//   void exportShape()
//   {
//     QString fn=QFileDialog::getSaveFileName
//     (
//       listWidget(), 
//       "Export file name", 
//       "", "BREP file (*,brep);;ASCII STL file (*.stl);;Binary STL file (*.stlb);;IGES file (*.igs);;STEP file (*.stp)"
//     );
//     if (!fn.isEmpty()) smp_->saveAs(qPrintable(fn));
//   }
//  
public slots:
  void showContextMenu(const QPoint& gpos) // this is a slot
  {
      // for QAbstractScrollArea and derived classes you would use:
      // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

      QMenu myMenu;
      myMenu.addAction("Shaded");
      myMenu.addAction("Wireframe");
      myMenu.addAction("Randomize Color");
//       myMenu.addAction("Export...");
      // ...

      QAction* selectedItem = myMenu.exec(gpos);
      if (selectedItem)
      {
	  if (selectedItem->text()=="Shaded") shaded();
	  if (selectedItem->text()=="Wireframe") wireframe();
	  if (selectedItem->text()=="Randomize Color") randomizeColor();
// 	  if (selectedItem->text()=="Export...") exportShape();
      }
      else
      {
	  // nothing was chosen
      }
  }
};

void DatumList::showContextMenuForWidget(const QPoint &p)
{
  QDatumItem * mi=dynamic_cast<QDatumItem*>(itemAt(p));
  if (mi)
  {
    mi->showContextMenu(this->mapToGlobal(p));
  }
}
  

class QEvaluationItem
: public QListWidgetItem
{
  EvaluationPtr smp_;
  QoccViewerContext* context_;
  Handle_AIS_InteractiveObject ais_;
    
public:
  ViewState state_;

  QEvaluationItem(const std::string& name, EvaluationPtr smp, QoccViewerContext* context, 
		 const ViewState& state, QListWidget* view = 0)
  : QListWidgetItem(QString::fromStdString(name), view),
    context_(context),
    state_(state)
  {
    setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
    reset(smp);
  }
  
  void reset(EvaluationPtr smp)
  {
    smp_=smp;
    if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
    ais_=smp_->createAISRepr();
    context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
    updateDisplay();
  }
  
  void wireframe()
  {
    state_.shading=0;
    updateDisplay();
  }

  void shaded()
  {
    state_.shading=1;
    updateDisplay();
  }
  
  void randomizeColor()
  {
    state_.randomizeColor();
    updateDisplay();
  }
  
  void updateDisplay()
  {
    state_.visible = (checkState()==Qt::Checked);
    
    if (state_.visible)
    {
      context_->getContext()->Display(ais_);
      context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
      context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
    }
    else
    {
      context_->getContext()->Erase(ais_);
    }
  }
  
//   void exportShape()
//   {
//     QString fn=QFileDialog::getSaveFileName
//     (
//       listWidget(), 
//       "Export file name", 
//       "", "BREP file (*,brep);;ASCII STL file (*.stl);;Binary STL file (*.stlb);;IGES file (*.igs);;STEP file (*.stp)"
//     );
//     if (!fn.isEmpty()) smp_->saveAs(qPrintable(fn));
//   }
//  
public slots:
  void showContextMenu(const QPoint& gpos) // this is a slot
  {
      // for QAbstractScrollArea and derived classes you would use:
      // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

      QMenu myMenu;
      myMenu.addAction("Shaded");
      myMenu.addAction("Wireframe");
      myMenu.addAction("Randomize Color");
//       myMenu.addAction("Export...");
      // ...

      QAction* selectedItem = myMenu.exec(gpos);
      if (selectedItem)
      {
	  if (selectedItem->text()=="Shaded") shaded();
	  if (selectedItem->text()=="Wireframe") wireframe();
	  if (selectedItem->text()=="Randomize Color") randomizeColor();
// 	  if (selectedItem->text()=="Export...") exportShape();
      }
      else
      {
	  // nothing was chosen
      }
  }
};

QVariableItem::QVariableItem(const std::string& name, arma::mat vv, QoccViewerContext* context, 
		const ViewState& state, QListWidget* view )
: QListWidgetItem
  (
   QString::fromStdString(name+" = ["+lexical_cast<string>(vv(0))+", "+lexical_cast<string>(vv(1))+", "+lexical_cast<string>(vv(2))+"]"), 
   view
  ),
  name_(QString::fromStdString(name)),
  context_(context),
  state_(state)
{
  setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
  reset(vv);
}

QVariableItem::createAISShape()
{
//   TopoDS_Edge cP = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(value_),gp_Dir(0,0,1)), 1));
//   Handle_AIS_Shape aisP = new AIS_Shape(cP);
  TopoDS_Shape cP=BRepBuilderAPI_MakeVertex(to_Pnt(value_));
  Handle_AIS_Shape aisP(new AIS_Shape(cP));
  
//   Handle_AIS_InteractiveObject aisPLabel (createArrow(
//     cP, name_.toStdString())
//   );
// 
//   std::auto_ptr<AIS_MultipleConnectedInteractive> ais(new AIS_MultipleConnectedInteractive());
// 
//   Handle_Standard_Transient owner_container(new PointerTransient(this));
//   aisP->SetOwner(owner_container);
//   aisPLabel->SetOwner(owner_container);
//   ais->Connect(aisP);
//   ais->Connect(aisPLabel);
//   ais_=ais.release();
  ais_=aisP;
}

void QVariableItem::reset(arma::mat val)
{
  value_=val;
  if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
  createAISShape();
//     Handle_Standard_Transient owner_container(new SolidModelTransient(smp));
//   context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
  updateDisplay();
}

void QVariableItem::updateDisplay()
{
  state_.visible = (checkState()==Qt::Checked);
  
  if (state_.visible)
  {
    context_->getContext()->Display(ais_);
//     context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
//     context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
  }
  else
  {
    context_->getContext()->Erase(ais_);
  }
}

void QVariableItem::insertName()
{
  emit insertParserStatementAtCursor(name_);
}

void QVariableItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    // for QAbstractScrollArea and derived classes you would use:
    // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

    QMenu myMenu;
    QAction *tit=new QAction(name_, &myMenu);
    tit->setDisabled(true);
    myMenu.addAction(tit);
    myMenu.addSeparator();
    myMenu.addAction("Insert name");
    // ...

    QAction* selectedItem = myMenu.exec(gpos);
    if (selectedItem)
    {
	if (selectedItem->text()=="Insert name") insertName();
    }
    else
    {
	// nothing was chosen
    }
}



void EvaluationList::showContextMenuForWidget(const QPoint &p)
{
  QEvaluationItem * mi=dynamic_cast<QEvaluationItem*>(itemAt(p));
  if (mi)
  {
    mi->showContextMenu(this->mapToGlobal(p));
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
void ISCADMainWindow::addModelStep(std::string sn, insight::cad::SolidModelPtr sm)
{ 
  ViewState vd;
  
  if (sm->isleaf()) vd.visible=true; else vd.visible=false;
  
  if (checked_modelsteps_.find(sn)!=checked_modelsteps_.end())
  {
    vd=checked_modelsteps_.find(sn)->second;
  }
  
  QModelStepItem* msi=new QModelStepItem(sn, sm, context_, vd);
  connect
  (
    msi, SIGNAL(insertParserStatementAtCursor(const QString&)),
    editor_, SLOT(insertPlainText(const QString&))
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

void ISCADMainWindow::addEvaluation(std::string sn, insight::cad::EvaluationPtr sm)
{ 
  ViewState vd;
  vd.visible=false;
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
      QString::fromStdString(sn+" = "+lexical_cast<string>(sv))
    )
  );
}

void ISCADMainWindow::addVariable(std::string sn, insight::cad::parser::vector vv)
{
//   variablelist_->addItem
//   (
//     new QListWidgetItem
//     (
//       QString::fromStdString(sn+" = ["+lexical_cast<string>(vv(0))+", "+lexical_cast<string>(vv(1))+", "+lexical_cast<string>(vv(2))+"]")
//     )
//   );
  ViewState vd;
  vd.visible=false;
  
//   if (checked_modelsteps_.find(sn)!=checked_modelsteps_.end())
//   {
//     vd=checked_modelsteps_.find(sn)->second;
//   }
  
  QVariableItem* msi=new QVariableItem(sn, vv, context_, vd);
  connect
  (
    msi, SIGNAL(insertParserStatementAtCursor(const QString&)),
    editor_, SLOT(insertPlainText(const QString&))
  );
  variablelist_->addItem(msi);
}

void ISCADMainWindow::rebuildModel()
{
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
    /*
  std::string code=editor_->toPlainText().toStdString();
  
  parser::model m;
  typedef std::string::iterator Iterator;
  Iterator orgbegin, begin;
  orgbegin=begin=code.begin();
  Iterator end=code.end();
  ISCADParser<Iterator> parser;
  skip_grammar<std::string::iterator> skip;
  
  bool r = qi::phrase_parse(
      begin,
      end,
      parser,
      skip
  );
  */
    
  std::istringstream is(editor_->toPlainText().toStdString());
  
  int failloc=-1;
  parser::Model::Ptr m(new Model);
  bool r=parseISCADModelStream(is, m, &failloc);

  if (!r) // fail if we did not get a full match
  {
    QTextCursor tmpCursor = editor_->textCursor();
    tmpCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1 );
    tmpCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, failloc );
    editor_->setTextCursor(tmpCursor);
    
    statusBar()->showMessage("Model regeneration failed => Cursor moved to location where parsing stopped!");
  }
  else
  {
    statusBar()->showMessage("Model regeneration successful.");
    
    context_->getContext()->EraseAll();
//     m->modelstepSymbols.for_each(Transferrer(*this));
    BOOST_FOREACH(const Model::modelstepSymbolTable::value_type& v, m->modelstepSymbols())
    { addModelStep(v.first, v.second); }
    BOOST_FOREACH(const Model::datumSymbolTable::value_type& v, m->datumSymbols())
    { addDatum(v.first, v.second); }
    BOOST_FOREACH(const Model::evaluationSymbolTable::value_type& v, m->evaluationSymbols())
    { addEvaluation(v.first, v.second); }
   
//     for (SolidModel::Map::const_iterator i=m->modelstepSymbols.begin();
// 	 i!=m->modelstepSymbols.end(); i++)
// 	 {
// 	   cout<<"inserting "<<i->first<<endl;
// 	   this->addModelStep(i->first, i->second);
// 	 }
//     m->scalarSymbols.for_each(Transferrer(*this));
    BOOST_FOREACH(const Model::scalarSymbolTable::value_type& v, m->scalarSymbols())
    { addVariable(v.first, v.second); }
    BOOST_FOREACH(const Model::vectorSymbolTable::value_type& v, m->vectorSymbols())
    { addVariable(v.first, v.second); }
//     m->vectorSymbols.for_each(Transferrer(*this));
  }
}

void ISCADMainWindow::popupMenu( const QoccViewWidget* aView, const QPoint aPoint )
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