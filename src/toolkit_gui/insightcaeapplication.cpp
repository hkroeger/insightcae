#include "insightcaeapplication.h"

#include <QMessageBox>

#include <iostream>

#include "qinsighterror.h"





InsightCAEApplication::InsightCAEApplication(int &argc, char **argv, const QString& appname)
: QApplication(argc, argv)
{
  setOrganizationName("silentdynamics");
  setApplicationName(appname);
}




InsightCAEApplication::~InsightCAEApplication()
{}




bool InsightCAEApplication::notify(QObject *rec, QEvent *ev)
{

  try
  {
    return QApplication::notify(rec, ev);
  }

  catch (const std::exception& e)
  {
    displayException(e);
  }

  return true;
}



