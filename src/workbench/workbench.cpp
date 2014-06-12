#include "workbench.h"

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QMessageBox>
#include <QFileDialog>

#include "newanalysisdlg.h"
#include "analysisform.h"

#include <fstream>
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

WorkbenchApplication::WorkbenchApplication(int &argc, char **argv)
: QApplication(argc, argv)
{
  connect(this, SIGNAL(exceptionOcurred(QString, QString)), this, SLOT(displayExceptionNotification(QString, QString)));
}

WorkbenchApplication::~WorkbenchApplication()
{}

bool WorkbenchApplication::notify(QObject *rec, QEvent *ev)
{
  try
  {
    return QApplication::notify(rec, ev);
  }
  catch (insight::Exception e)
  {
    std::cout << e << std::endl;
    
    emit exceptionOcurred
    (
      QString(e.message().c_str()), 
      QString(e.strace().c_str())
    );
    
//     QMessageBox msgBox;
//     msgBox.setIcon(QMessageBox::Critical);
//     msgBox.setText(QString(e.as_string().c_str()));
// /*    if (e.addInfo()!="")
//     {
//       msgBox.setInformativeText("Please check additional info.");
//       msgBox.setDetailedText(QString(e.addInfo().c_str()));
//     }*/
//     msgBox.exec();
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

void WorkbenchApplication::displayExceptionNotification(QString msg, QString addinfo)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(msg);
    if (addinfo!="")
    {
      msgBox.setInformativeText("See details for stack trace of error origin.");
      msgBox.setDetailedText(addinfo);
    }
    msgBox.exec();
}

workbench::workbench()
{
  this->setWindowTitle("Insight Workbench");
    mdiArea_ = new QMdiArea(this);
    setCentralWidget( mdiArea_ );
    
    QMenu *analysisMenu = menuBar()->addMenu( "&Analysis" );

    QAction* a = new QAction("New...", this); 
    a->setShortcut(Qt::ControlModifier + Qt::Key_N);
    connect(a, SIGNAL(triggered()), SLOT(newAnalysis()) );
    analysisMenu->addAction( a );
    
    a = new QAction("Open...", this); 
    a->setShortcut(Qt::ControlModifier + Qt::Key_O);
    connect(a, SIGNAL(triggered()), SLOT(onOpenAnalysis()) );
    analysisMenu->addAction( a );
}

workbench::~workbench()
{}

void workbench::newAnalysis()
{
  newAnalysisDlg dlg(this);
  if (dlg.exec() == QDialog::Accepted)
  {
    AnalysisForm *form= new AnalysisForm(mdiArea_, dlg.getAnalysisName());
    form->showMaximized();
  }
}

void workbench::onOpenAnalysis()
{
  QString fn = QFileDialog::getOpenFileName(this, "Open Parameters", QString(), "Insight parameter sets (*.ist)");
  if (!fn.isEmpty()) openAnalysis(fn);
}
    
void workbench::openAnalysis(const QString& fn)
{
  using namespace rapidxml;
  
  boost::filesystem::path fp(fn.toStdString());
  
  std::ifstream in(fp.c_str());
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();

  xml_document<> doc;
  doc.parse<0>(&contents[0]);
  
  xml_node<> *rootnode = doc.first_node("root");
  
  std::string analysisName;
  xml_node<> *analysisnamenode = rootnode->first_node("analysis");
  if (analysisnamenode)
  {
    analysisName = analysisnamenode->first_attribute("name")->value();
  }
  
  AnalysisForm *form= new AnalysisForm(mdiArea_, analysisName);
  form->parameters().readFromNode(doc, *rootnode, fp.parent_path());
  boost::filesystem::path dir=boost::filesystem::path(fn.toStdString()).parent_path();
  form->analysis().setExecutionPath(dir);
  form->forceUpdate();
  form->showMaximized();
}

//#include "workbench.moc"
