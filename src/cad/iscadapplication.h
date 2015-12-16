/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef ISCADAPPLICATION_H
#define ISCADAPPLICATION_H

#include <QApplication>
#include <QMainWindow>
#include <QMdiArea>
#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>

#ifndef Q_MOC_RUN
#include "occinclude.h"
#include "solidmodel.h"
#include "parser.h"
#endif

struct ViewState
{
  int shading;
  bool visible;
  double r, g, b;
  
  ViewState();
  void randomizeColor();
};


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

class QModelStepItem
: public QObject, public QListWidgetItem
{
  Q_OBJECT 
  
  QString name_;
  insight::cad::SolidModelPtr smp_;
  QoccViewerContext* context_;
  Handle_AIS_Shape ais_;
    
signals:
  void insertParserStatementAtCursor(const QString& statement);
 
public:
  ViewState state_;

  QModelStepItem(const std::string& name, insight::cad::SolidModelPtr smp, QoccViewerContext* context, 
		 const ViewState& state, QListWidget* view = 0);
  
  void reset(insight::cad::SolidModelPtr smp);
  void wireframe();
  void shaded();
  void hide();
  void show();
  void randomizeColor();
  void updateDisplay();
  void exportShape();
  void insertName();
  
public slots:
  void showContextMenu(const QPoint& gpos);
};

class QVariableItem
: public QObject, public QListWidgetItem
{
  Q_OBJECT 
  
  QString name_;
  arma::mat value_;
  QoccViewerContext* context_;
  Handle_AIS_InteractiveObject ais_;
    
signals:
  void insertParserStatementAtCursor(const QString& statement);
 
protected:
  void createAISShape();
  
public:
  ViewState state_;

  QVariableItem(const std::string& name, arma::mat value, 
		QoccViewerContext* context, 
		const ViewState& state, QListWidget* view = 0);
  
  void reset(arma::mat value);
  void updateDisplay();
  void insertName();
  
public slots:
  void showContextMenu(const QPoint& gpos);
};


class ModelStepList
: public QListWidget
{
  Q_OBJECT
  
public:
  ModelStepList(QWidget* parent = 0);
  
protected slots:
  void showContextMenuForWidget(const QPoint &);
};

class DatumList
: public QListWidget
{
  Q_OBJECT
  
public:
  DatumList(QWidget* parent = 0);
  
protected slots:
  void showContextMenuForWidget(const QPoint &);
};

class EvaluationList
: public QListWidget
{
  Q_OBJECT
  
public:
  EvaluationList(QWidget* parent = 0);
  
protected slots:
  void showContextMenuForWidget(const QPoint &);
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
  void onVariableItemChanged(QListWidgetItem * item);
  void onModelStepItemChanged(QListWidgetItem * item);
  void onDatumItemChanged(QListWidgetItem * item);
  void onEvaluationItemChanged(QListWidgetItem * item);

public:
  ISCADMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  
  // insert model step
  void addModelStep(std::string sn, insight::cad::SolidModelPtr sm);
  void addDatum(std::string sn, insight::cad::DatumPtr dm);
  void addEvaluation(std::string sn, insight::cad::EvaluationPtr em);
  void addVariable(std::string sn, insight::cad::parser::scalar sv);
  void addVariable(std::string sn, insight::cad::parser::vector vv);
  
  void loadFile(const boost::filesystem::path& file);

public slots:
  void loadModel();
  void saveModel();
  void saveModelAs();
  void rebuildModel();
  void popupMenu( const QoccViewWidget* aView, const QPoint aPoint ); 
  
protected:
  boost::filesystem::path filename_;
  QoccViewerContext* context_;
  QoccViewWidget* viewer_;
  QListWidget* modelsteplist_;
  QListWidget* datumlist_;
  QListWidget* evaluationlist_;
  QListWidget* variablelist_;
  QTextEdit* editor_;
  
  std::map<std::string, ViewState> checked_modelsteps_, checked_datums_, checked_evaluations_;

};

#endif
