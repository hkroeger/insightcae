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

#ifndef QDATUMITEM_H
#define QDATUMITEM_H

#include <QListWidgetItem>
#include "qmodeltree.h"

#ifndef Q_MOC_RUN
#include "occinclude.h"
#include "cadfeature.h"
#endif

class QoccViewerContext;

class QDatumItem
: public QDisplayableModelTreeItem
{
  Q_OBJECT
  
  insight::cad::DatumPtr smp_;
    
protected:
  virtual Handle_AIS_InteractiveObject createAIS(AIS_InteractiveContext& context);

public:
  QDatumItem(const QString& name, insight::cad::DatumPtr smp, QTreeWidgetItem* parent);
  
  inline insight::cad::DatumPtr datum() const { return smp_; }

public slots:
  void showContextMenu(const QPoint& gpos);
  
};


#endif // QDATUMITEM_H
