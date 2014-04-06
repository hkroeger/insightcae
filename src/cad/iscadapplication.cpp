#include "iscadapplication.h"
#include "base/exception.h"
#include "qoccviewercontext.h"
#include "qoccviewwidget.h"

#include <iostream>

#include <QMessageBox>
#include <QMainWindow>


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

ISCADMainWindow::ISCADMainWindow(QWidget* parent, Qt::WindowFlags flags)
: QMainWindow(parent, flags)
{
  QSplitter *spl=new QSplitter(Qt::Horizontal);
  setCentralWidget(spl);
  context_=new QoccViewerContext;
  viewer_=new QoccViewWidget(context_->getContext(), spl);
  spl->addWidget(viewer_);
  QTextEdit* ed=new QTextEdit;
  spl->addWidget(ed);
}
