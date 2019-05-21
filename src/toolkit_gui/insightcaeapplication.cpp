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
    std::cout << "InsightCAE exception occurred:\n" << e << std::endl;

    emit exceptionOcurred
    (
      QString(e.message().c_str()),
      QString(e.strace().c_str())
    );
  }
  catch (std::exception e)
  {
    std::cout << "Unhandled STL exception occurred:\n" << e.what() << std::endl;

    emit exceptionOcurred
    (
      QString(e.what())
    );
  }

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

