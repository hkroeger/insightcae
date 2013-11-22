#include "workbench.h"

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QMessageBox>

#include "newanalysisdlg.h"
#include "analysisform.h"

WorkbenchApplication::WorkbenchApplication(int &argc, char **argv)
: QApplication(argc, argv)
{}

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

#include "workbench.moc"
