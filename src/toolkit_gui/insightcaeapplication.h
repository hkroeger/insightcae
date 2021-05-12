#ifndef INSIGHTCAEAPPLICATION_H
#define INSIGHTCAEAPPLICATION_H

#ifdef QT_STATICPLUGIN
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

#include <QApplication>

class QSplashScreen;

class InsightCAEApplication
: public QApplication
{
  Q_OBJECT

  QSplashScreen* sc_;

public:

  InsightCAEApplication(
      int &argc, char **argv,
      const QString& appname
      );
  ~InsightCAEApplication( );

  void setSplashScreen(QSplashScreen *sc);

  bool notify(QObject *rec, QEvent *ev);

};


#endif // INSIGHTCAEAPPLICATION_H
