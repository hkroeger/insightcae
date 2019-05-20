#include "insightcaeapplication.h"

#include <QMessageBox>

#include <iostream>

#include "base/exception.h"





InsightCAEApplication::InsightCAEApplication(int &argc, char **argv)
: QApplication(argc, argv)
{
  connect(this, &InsightCAEApplication::exceptionOcurred,
          this, &InsightCAEApplication::displayExceptionNotification);
}




InsightCAEApplication::~InsightCAEApplication()
{}




bool InsightCAEApplication::notify(QObject *rec, QEvent *ev)
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




void InsightCAEApplication::displayExceptionNotification(QString msg, QString addinfo)
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

