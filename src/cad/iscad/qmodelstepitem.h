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

#include "qmodeltree.h"

#ifndef Q_MOC_RUN
#include "occinclude.h"
#include "cadfeature.h"
#endif

class ISCADMainWindow;
class QoccViewerContext;
class QFeatureItem;


// class QFeatureItemAdder
// : public QThread
// {
//   ISCADMainWindow* mw_;
//   QFeatureItem* msi_;
//   
// public:
//   QFeatureItemAdder
//   (
//     ISCADMainWindow* mw, 
//     QFeatureItem* msi
//   );
//   
//   void run();
// };




class QFeatureItem
: public QDisplayableModelTreeItem
{
//   friend class QFeatureItemAdder;
  
  Q_OBJECT 
  
  insight::cad::FeaturePtr smp_;
  Handle_AIS_Shape ais_;
  bool is_component_;
    
signals:
  void jump_to(const QString& name);
  void insertParserStatementAtCursor(const QString& statement);
  void setUniformDisplayMode(const AIS_DisplayMode AM);
  void addEvaluation(std::string sn, insight::cad::PostprocActionPtr em, bool visible);

public:
  QFeatureItem(const std::string& name, insight::cad::FeaturePtr smp, QoccViewerContext* context, 
		 const ViewState& state, QTreeWidgetItem* parent, bool is_component);
  
//   void run();
  void reset(insight::cad::FeaturePtr smp);
  
  void rebuild();
  void updateDisplay();
  void resetDisplay();
  
  inline insight::cad::Feature& solidmodel()
  {
    return *smp_;
  }
  
public slots:
  void jump();
  void wireframe();
  void shaded();
  void onlyThisShaded();
  void hide();
  void show();
  void randomizeColor();
  void showProperties();
  void exportShape();
  void setResolution();
  void insertName();
  void showContextMenu(const QPoint& gpos);
};




#endif // QMODELSTEPITEM_H
