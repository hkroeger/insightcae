#ifndef INSIGHTCAEAPPLICATION_H
#define INSIGHTCAEAPPLICATION_H


#include <QApplication>


class InsightCAEApplication
: public QApplication
{
  Q_OBJECT

public:

  InsightCAEApplication( int &argc, char **argv);
  ~InsightCAEApplication( );

  bool notify(QObject *rec, QEvent *ev);

signals:
  void exceptionOcurred(QString msg, QString addinfo="");

public slots:
  void displayExceptionNotification(QString msg, QString addinfo="");

};


#endif // INSIGHTCAEAPPLICATION_H
