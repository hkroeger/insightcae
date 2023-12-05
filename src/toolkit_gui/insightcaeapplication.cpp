#include "insightcaeapplication.h"

#include <QMessageBox>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSplashScreen>
#include <QTimer>
#include <QDebug>

#include <iostream>

#include "qinsighterror.h"





InsightCAEApplication::InsightCAEApplication(
    int &argc, char **argv,
    const QString& appname)
: QApplication(argc, argv), sc_(nullptr)
{
  setOrganizationName("silentdynamics");
  setApplicationName(appname);

  setlocale(LC_NUMERIC, "C");

  // auto qloc = QLocale::system(); // can't change only the decimal...
  // qDebug()<<"DECIMAL="<<qloc.decimalPoint();
  QLocale::setDefault(QLocale::c());
}




InsightCAEApplication::~InsightCAEApplication()
{}

void InsightCAEApplication::setSplashScreen(QSplashScreen *sc)
{
  sc_=sc;
  QTimer::singleShot(3000, sc, &QSplashScreen::close);
//  connect( sc, &QSplashScreen::close,
//          [&]() { sc_=nullptr; } );
}




bool InsightCAEApplication::notify(QObject *rec, QEvent *ev)
{
  if (sc_)
  {
    if (
        dynamic_cast<QKeyEvent*>(ev) ||
       (dynamic_cast<QMouseEvent*>(ev)&& ev->type()!=QEvent::MouseMove)
        )
    {
      sc_->close();
      sc_=nullptr;
    }
  }

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



