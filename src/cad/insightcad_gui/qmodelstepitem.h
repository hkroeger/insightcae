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

#undef Bool

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



class QFeatureItem
: public QDisplayableModelTreeItem
{
//   friend class QFeatureItemAdder;
  
  Q_OBJECT 
  
  insight::cad::FeaturePtr smp_;
  bool is_component_;
    
Q_SIGNALS:
  void addEvaluation(const QString& name, insight::cad::PostprocActionPtr em, bool visible);

protected:
  virtual Handle_AIS_InteractiveObject createAIS(AIS_InteractiveContext& context);
  void addSymbolsToSubmenu(const QString& name, QMenu *menu, insight::cad::FeaturePtr feat, bool *someSubMenu=NULL, bool *someHoverDisplay=NULL);

public:
  QFeatureItem(const QString& name, insight::cad::FeaturePtr smp,
         bool visible, QTreeWidgetItem* parent, bool is_component);
  
  inline insight::cad::FeaturePtr solidmodelPtr()
  {
    return smp_;
  }

  inline insight::cad::Feature& solidmodel()
  {
    return *smp_;
  }
  
public Q_SLOTS:
  void showProperties();
  void exportShape();

  void showContextMenu(const QPoint& gpos);
};




#endif // QMODELSTEPITEM_H
