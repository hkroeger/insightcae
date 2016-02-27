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

#include "datum.h"
#include "cadpostprocactions.h"

#include "qoccviewwidget.h"
#include "qoccviewercontext.h"
#include "occtools.h"

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

    msgBox.exec();
  }

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
    Feature& sm=ms->solidmodel();
    const Feature::RefPointsList& pts=sm.getDatumPoints();
    
    // reverse storage to detect collocated points
    typedef std::map<arma::mat, std::string, compareArmaMat> trpts;
    trpts rpts;
    BOOST_FOREACH(const Feature::RefPointsList::value_type& p, pts)
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
      Handle_AIS_InteractiveObject o(new InteractiveText(name, xyz));
      additionalDisplayObjectsForSelection_.push_back(o);
      aView->getContext()->Display(o, false);
    }
  }
  
  aView->getContext()->UpdateCurrentViewer();
}


ISCADMainWindow::ISCADMainWindow(QWidget* parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags)
{  
  QSplitter *spl=new QSplitter(Qt::Horizontal);
  setCentralWidget(spl);
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
  
  highlighter_=new ISCADHighlighter(editor_->document());
  
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
  act = new QAction(("Toggle &clip plane"), this);
  connect(act, SIGNAL(triggered()), viewer_, SLOT(toggleClip()));
  vmenu->addAction(act);
  act = new QAction(("Change background color..."), this);
  connect(act, SIGNAL(triggered()), viewer_, SLOT(background()));
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

// /**
//  * Needed since QMainWindows copy constructor is not available
//  */
// struct Transferrer
// {
//   ISCADMainWindow& mw_;
//   
//   Transferrer(ISCADMainWindow& mw)
//   : mw_(mw)
//   {
//   }
//   
//   void operator()(std::string sn, insight::cad::FeaturePtr sm)
//   {
//     cout<<sn<<" : "<<sm.get()<<endl;
//     mw_.addModelStep(sn, sm);
//   }
// 
//   void operator()(std::string sn, insight::cad::parser::scalar sv)
//   {
//     mw_.addVariable(sn, sv);
//   }
// 
//   void operator()(std::string sn, insight::cad::parser::vector vv)
//   {
//     mw_.addVariable(sn, vv);
//   }
// };
// 

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




QModelStepItem::QModelStepItem(const std::string& name, FeaturePtr smp, QoccViewerContext* context, 
		const ViewState& state, QListWidget* view )
: QListWidgetItem(QString::fromStdString(name), view),
  name_(QString::fromStdString(name)),
  context_(context),
  state_(state)
{
  setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
  reset(smp);
}

void QModelStepItem::reset(FeaturePtr smp)
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

void QModelStepItem::onlyThisShaded()
{
//   qDebug()<<"all wireframe"<<endl;
  
  emit(setUniformDisplayMode(AIS_WireFrame));
  shaded();
}


void QModelStepItem::hide()
{
  setCheckState(Qt::Unchecked);
  updateDisplay();
}

void QModelStepItem::show()
{
  setCheckState(Qt::Checked);
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
    QMenu myMenu;
    QAction *tit=new QAction(name_, &myMenu);
//     tit->setDisabled(true);
    myMenu.addAction(tit);
    myMenu.addSeparator();
    myMenu.addAction("Insert name");
    myMenu.addSeparator();
    myMenu.addAction("Show");
    myMenu.addAction("Hide");
    myMenu.addAction("Shaded");
    myMenu.addAction("Only this shaded");
    myMenu.addAction("Wireframe");
    myMenu.addAction("Randomize Color");
    myMenu.addAction("Export...");

    QAction* selectedItem = myMenu.exec(gpos);
    if (selectedItem)
    {
	if (selectedItem->text()==name_) emit(jump_to(name_));
	if (selectedItem->text()=="Show") show();
	if (selectedItem->text()=="Hide") hide();
	if (selectedItem->text()=="Shaded") shaded();
	if (selectedItem->text()=="Wireframe") wireframe();
	if (selectedItem->text()=="Only this shaded") onlyThisShaded();
	if (selectedItem->text()=="Randomize Color") randomizeColor();
	if (selectedItem->text()=="Insert name") insertName();
	if (selectedItem->text()=="Export...") exportShape();
    }
    else
    {
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
  PostprocActionPtr smp_;
  QoccViewerContext* context_;
  Handle_AIS_InteractiveObject ais_;
    
public:
  ViewState state_;

  QEvaluationItem(const std::string& name, PostprocActionPtr smp, QoccViewerContext* context, 
		 const ViewState& state, QListWidget* view = 0)
  : QListWidgetItem(QString::fromStdString(name), view),
    context_(context),
    state_(state)
  {
    setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
    reset(smp);
  }
  
  void reset(PostprocActionPtr smp)
  {
    smp_=smp;
    if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
    ais_=smp_->createAISRepr();
    if (!ais_.IsNull()) 
    {
      context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
    }
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
  
  void hide()
  {
    setCheckState(Qt::Unchecked);
    updateDisplay();
  }
  
  void show()
  {
    setCheckState(Qt::Checked);
    updateDisplay();
  }
  
  void updateDisplay()
  {
    state_.visible = (checkState()==Qt::Checked);
    
    if (!ais_.IsNull())
    {
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

void QVariableItem::createAISShape()
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
  

  
  
ISCADHighlighter::ISCADHighlighter(QTextDocument* parent)
: QSyntaxHighlighter(parent)
{
  highlightingRules.resize(HighlightingRule_Index_Max);
  
  QString ident_pat("[a-zA-Z][a-zA-Z0-9_]*");
  
  {
    HighlightingRule rule;
    rule.pattern=QRegExp();
    rule.format.setForeground(Qt::darkBlue);
    rule.format.setBackground(Qt::yellow);
    rule.format.setFontWeight(QFont::Bold);
    
    highlightingRules[HighlightingRule_SelectedKeyword]=rule;
  }

  {
    HighlightingRule rule;
    rule.pattern=QRegExp("(#.*)$");
    rule.format.setForeground(Qt::gray);    
    rule.format.setBackground(Qt::white);
    rule.format.setFontWeight(QFont::Normal);
    highlightingRules[HighlightingRule_CommentHash]=rule;
  }

  {
    HighlightingRule rule;
    rule.pattern=QRegExp("\\b("+ident_pat+")\\b *\\(");
    rule.format.setForeground(Qt::darkBlue);
    rule.format.setBackground(Qt::white);
    rule.format.setFontWeight(QFont::Bold);
    
    highlightingRules[HighlightingRule_Function]=rule;
  }
  
  {
    HighlightingRule rule;
    rule.pattern=QRegExp("\\b("+ident_pat+")\\b *\\:");
    rule.format.setForeground(Qt::darkRed);
    rule.format.setBackground(Qt::white);
    rule.format.setFontWeight(QFont::Bold);
    
    highlightingRules[HighlightingRule_ModelStepDef]=rule;
  }
}

void ISCADHighlighter::setHighlightWord(const QString& word)
{
//   qDebug()<<"setting highlight word = "<<word<<endl;
  
  if (word.isEmpty())
  {
    highlightingRules[HighlightingRule_SelectedKeyword].pattern=QRegExp();
  }
  else
  { 
    highlightingRules[HighlightingRule_SelectedKeyword].pattern=QRegExp("\\b("+word+")\\b");
  }
}



void ISCADHighlighter::highlightBlock(const QString& text)
{
  foreach (const HighlightingRule &rule, highlightingRules) 
  {
    if (!rule.pattern.isEmpty())
    {
      QRegExp expression(rule.pattern);
      
      int index0 = expression.indexIn(text);
      int index = expression.pos(1);
      
      while (index0 >= 0) 
      {
 	int fulllength = expression.matchedLength();
	int length = expression.cap(1).length();
	setFormat(index0, length, rule.format);
	
	index0 = expression.indexIn(text, index0 + fulllength);
	index = expression.pos(1);
      }
    }
  }
}

void ISCADMainWindow::onEditorSelectionChanged()
{
  QTextDocument *doc = editor_->document();
  QString word=editor_->textCursor().selectedText();
  highlighter_->setHighlightWord(word);
  highlighter_->rehighlight();
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

void ISCADMainWindow::addEvaluation(std::string sn, insight::cad::PostprocActionPtr sm)
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
      QString::fromStdString(sn+" = "+lexical_cast<string>(sv->value()))
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
  ModelPtr m(new Model);
  bool r=parseISCADModelStream(is, m.get(), &failloc);

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
   
//     for (SolidModel::Map::const_iterator i=m->modelstepSymbols.begin();
// 	 i!=m->modelstepSymbols.end(); i++)
// 	 {
// 	   cout<<"inserting "<<i->first<<endl;
// 	   this->addModelStep(i->first, i->second);
// 	 }
//     m->scalarSymbols.for_each(Transferrer(*this));

    auto scalars=m->scalars();
    BOOST_FOREACH(decltype(scalars)::value_type const& v, scalars)
    { addVariable(v.first, v.second); }
    
    auto vectors=m->vectors();
    BOOST_FOREACH(decltype(vectors)::value_type const& v, vectors)
    { addVariable(v.first, v.second); }
//     m->vectorSymbols.for_each(Transferrer(*this));
  }
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