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

#ifndef QVARIABLEITEM_H
#define QVARIABLEITEM_H

#include <QObject>
#include <QListWidgetItem>

#include "viewstate.h"

#ifndef Q_MOC_RUN
#include "occinclude.h"
#include "cadfeature.h"
#endif

class QoccViewerContext;

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


#endif // QVARIABLEITEM_H
