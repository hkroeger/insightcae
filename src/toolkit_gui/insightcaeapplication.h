#ifndef INSIGHTCAEAPPLICATION_H
#define INSIGHTCAEAPPLICATION_H


#include <QApplication>


class InsightCAEApplication
: public QApplication
{
  Q_OBJECT

public:

  InsightCAEApplication( int &argc, char **argv, const QString& appname);
  ~InsightCAEApplication( );

  bool notify(QObject *rec, QEvent *ev);

};


#endif // INSIGHTCAEAPPLICATION_H
