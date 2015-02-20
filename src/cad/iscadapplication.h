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

class DatumList
: public QListWidget
{
  Q_OBJECT
  
public:
  DatumList(QWidget* parent = 0);
  
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
  void onDatumItemChanged(QListWidgetItem * item);

public:
  ISCADMainWindow(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  
  // insert model step
  void addModelStep(std::string sn, insight::cad::SolidModel::Ptr sm);
  void addDatum(std::string sn, insight::cad::Datum::Ptr dm);
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
  QListWidget* datumlist_;
  QListWidget* variablelist_;
  QTextEdit* editor_;
  
  std::map<std::string, ViewState> checked_modelsteps_, checked_datums_;

};

#endif