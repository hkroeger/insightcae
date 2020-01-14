#ifndef INSIGHTCAEAPPLICATION_H
#define INSIGHTCAEAPPLICATION_H


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
