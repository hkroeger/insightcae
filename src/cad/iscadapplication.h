#ifndef ISCADAPPLICATION_H
#define ISCADAPPLICATION_H

#include <QApplication>
#include <QMainWindow>
#include <QMdiArea>
#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>


#include "parser.h"

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


class ModelStepList
: public QListWidget
{
  Q_OBJECT
  
public:
  ModelStepList(QWidget* parent = 0);
  
protected slots:
  void showContextMenuForWidget(const QPoint &);
};

struct ViewState
{
  int shading;
  bool visible;
  double r, g, b;
  
  ViewState();
  void randomizeColor();
};


class ISCADMainWindow
: public QMainWindow
{
  Q_OBJECT
  
protected:
  void clearDerivedData();
  virtual void closeEvent(QCloseEvent *event);
  
  inline void setFilename(const boost::filesystem::path& fn)
  {
    filename_=fn;
    setWindowTitle(filename_.filename().c_str());
  }

protected slots:
  void onModelStepItemChanged(QListWidgetItem * item);

public:
  ISCADMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  
  // insert model step
  void addModelStep(std::string sn, insight::cad::SolidModel::Ptr sm);
  void addVariable(std::string sn, insight::cad::parser::scalar sv);
  void addVariable(std::string sn, insight::cad::parser::vector vv);
  
  void loadFile(const boost::filesystem::path& file);

public slots:
  void loadModel();
  void saveModel();
  void saveModelAs();
  void rebuildModel();

protected:
  boost::filesystem::path filename_;
  QoccViewerContext* context_;
  QoccViewWidget* viewer_;
  QListWidget* modelsteplist_;
  QListWidget* variablelist_;
  QTextEdit* editor_;
  
  std::map<std::string, ViewState> checked_modelsteps_;

};

#endif