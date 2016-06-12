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
 */

#ifndef QMODELSTEPITEM_H
#define QMODELSTEPITEM_H

#include <QThread>
#include <QListWidgetItem>

#include "viewstate.h"

#ifndef Q_MOC_RUN
#include "occinclude.h"
#include "cadfeature.h"
#endif

class ISCADMainWindow;
class QModelStepItem;
class QoccViewerContext;

class ModelStepItemAdder
: public QThread
{
  ISCADMainWindow* mw_;
  QModelStepItem* msi_;
  
public:
  ModelStepItemAdder
  (
    ISCADMainWindow* mw, 
    QModelStepItem* msi
  );
  
  void run();
};




class QModelStepItem
: //public QObject, 
  public QObject, //QThread, 
  public QListWidgetItem
{
  friend class ModelStepItemAdder;
  
  Q_OBJECT 
  
  QString name_;
  insight::cad::FeaturePtr smp_;
  QoccViewerContext* context_;
  Handle_AIS_Shape ais_;
    
signals:
  void jump_to(const QString& name);
  void insertParserStatementAtCursor(const QString& statement);
  void setUniformDisplayMode(const AIS_DisplayMode AM);
  void addEvaluation(std::string sn, insight::cad::PostprocActionPtr em, bool visible);

public:
  ViewState state_;

  QModelStepItem(const std::string& name, insight::cad::FeaturePtr smp, QoccViewerContext* context, 
		 const ViewState& state, QListWidget* view = 0);
  
//   void run();
  void rebuild();
  void reset(insight::cad::FeaturePtr smp);
  void wireframe();
  void shaded();
  void onlyThisShaded();
  void hide();
  void show();
  void randomizeColor();
  void updateDisplay();
  void showProperties();
  void exportShape();
  void setResolution();
  void insertName();
  
  inline insight::cad::Feature& solidmodel()
  {
    return *smp_;
  }
  
public slots:
  void showContextMenu(const QPoint& gpos);
};




#endif // QMODELSTEPITEM_H
