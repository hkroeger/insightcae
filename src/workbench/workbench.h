#ifndef workbench_H
#define workbench_H

#include <QtGui/QMainWindow>
#include <QtGui/QMdiArea>

#include <QApplication>

class WorkbenchApplication
: public QApplication
{
  Q_OBJECT

public:

  WorkbenchApplication( int &argc, char **argv);
  ~WorkbenchApplication( );

  bool notify(QObject *rec, QEvent *ev);

};


class workbench
: public QMainWindow
{
Q_OBJECT
private:
  QMdiArea *mdiArea_;
  
public:
    workbench();
    virtual ~workbench();
    
private slots:
    void newAnalysis();
    void onOpenAnalysis();
};

#endif // workbench_H
