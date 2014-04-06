#ifndef ISCADAPPLICATION_H
#define ISCADAPPLICATION_H

#include <QApplication>
#include <QMainWindow>
#include <QMdiArea>
#include <QSplitter>

class ISCADApplication
: public QApplication
{
  Q_OBJECT

public:
  ISCADApplication( int &argc, char **argv);
  ~ISCADApplication( );

  bool notify(QObject *rec, QEvent *ev);
};

class QoccViewerContext;
class QoccViewWidget;


//class 

class ISCADMainWindow
: public QMainWindow
{
  Q_OBJECT
protected:
  QoccViewerContext* context_;
  QoccViewWidget* viewer_;
  
public:
  ISCADMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
};

#endif