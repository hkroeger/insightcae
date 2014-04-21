#include "iscadapplication.h"
#include "base/exception.h"
#include "qoccviewercontext.h"
#include "qoccviewwidget.h"
#include <iostream>

#include "occinclude.h"
#include "AIS.hxx"
#include "AIS_Shape.hxx"
#include "AIS_InteractiveContext.hxx"

#include <QMessageBox>
#include <QMainWindow>

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
    msgBox.setText(QString(e.c_str()));
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

ISCADMainWindow::ISCADMainWindow(QWidget* parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags)
{  
  QSplitter *spl=new QSplitter(Qt::Horizontal);
  setCentralWidget(spl);
  context_=new QoccViewerContext;
  
  viewer_=new QoccViewWidget(context_->getContext(), spl);
  spl->addWidget(viewer_);
  
  editor_=new QTextEdit(spl);
  spl->addWidget(editor_);
  
  QSplitter* spl2=new QSplitter(Qt::Vertical, spl);
  variablelist_=new QListWidget;
  spl2->addWidget(variablelist_);
  modelsteplist_=new ModelStepList;
  connect(modelsteplist_, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(onModelStepItemChanged(QListWidgetItem*)));
  spl2->addWidget(modelsteplist_);
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
  variablelist_->clear();
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
  
  void operator()(std::string sn, insight::cad::SolidModel::Ptr sm)
  {
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

class QModelStepItem
: public QListWidgetItem
{
  SolidModel::Ptr smp_;
  QoccViewerContext* context_;
  Handle_AIS_Shape ais_;
    
public:
  ViewState state_;

  QModelStepItem(const std::string& name, SolidModel::Ptr smp, QoccViewerContext* context, 
		 const ViewState& state, QListWidget* view = 0)
  : QListWidgetItem(QString::fromStdString(name), view),
    context_(context),
    state_(state)
  {
    setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
    reset(smp);
  }
  
  void reset(SolidModel::Ptr smp)
  {
    smp_=smp;
    if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
    ais_=new AIS_Shape(*smp_);
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
 
public slots:
  void showContextMenu(const QPoint& gpos) // this is a slot
  {
      // for QAbstractScrollArea and derived classes you would use:
      // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

      QMenu myMenu;
      myMenu.addAction("Shaded");
      myMenu.addAction("Wireframe");
      myMenu.addAction("Randomize Color");
      // ...

      QAction* selectedItem = myMenu.exec(gpos);
      if (selectedItem)
      {
	  if (selectedItem->text()=="Shaded") shaded();
	  if (selectedItem->text()=="Wireframe") wireframe();
	  if (selectedItem->text()=="Randomize Color") randomizeColor();
      }
      else
      {
	  // nothing was chosen
      }
  }
};

void ModelStepList::showContextMenuForWidget(const QPoint &p)
{
  QModelStepItem * mi=dynamic_cast<QModelStepItem*>(itemAt(p));
  if (mi)
  {
    mi->showContextMenu(this->mapToGlobal(p));
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

void ISCADMainWindow::addModelStep(std::string sn, insight::cad::SolidModel::Ptr sm)
{ 
  ViewState vd;
  
  if (sm->isleaf()) vd.visible=true; else vd.visible=false;
  
  if (checked_modelsteps_.find(sn)!=checked_modelsteps_.end())
  {
    vd=checked_modelsteps_.find(sn)->second;
  }
  
  modelsteplist_->addItem(new QModelStepItem(sn, sm, context_, vd));
}

void ISCADMainWindow::addVariable(std::string sn, insight::cad::parser::scalar sv)
{
  variablelist_->addItem(new QListWidgetItem(QString::fromStdString(sn+" = "+lexical_cast<string>(sv))));
}

void ISCADMainWindow::addVariable(std::string sn, insight::cad::parser::vector vv)
{
  variablelist_->addItem(new QListWidgetItem(QString::fromStdString(sn+" = ["+lexical_cast<string>(vv(0))+", "+lexical_cast<string>(vv(1))+", "+lexical_cast<string>(vv(2))+"]")));
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

  clearDerivedData();
    
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

  if (begin != end) // fail if we did not get a full match
  {
    QTextCursor tmpCursor = editor_->textCursor();
    tmpCursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1 );
    tmpCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, int(begin-orgbegin) );
    editor_->setTextCursor(tmpCursor);
    
    statusBar()->showMessage("Model regeneration failed => Cursor moved to location where parsing stopped!");
  }
  else
  {
    statusBar()->showMessage("Model regeneration successful.");
  }
    
  context_->getContext()->EraseAll();
  parser.modelstepSymbols.for_each(Transferrer(*this));
  parser.scalarSymbols.for_each(Transferrer(*this));
  parser.vectorSymbols.for_each(Transferrer(*this));
  
}
