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

#ifndef QEVALUATIONITEM_H
#define QEVALUATIONITEM_H

#include <QListWidgetItem>
#include "viewstate.h"

#ifndef Q_MOC_RUN
#include "occinclude.h"
// #include "cadtypes.h"
#include "cadfeature.h"
#endif
class QoccViewerContext;

class QEvaluationItem
: public QListWidgetItem
{
  insight::cad::PostprocActionPtr smp_;
  QoccViewerContext* context_;
  Handle_AIS_InteractiveObject ais_;
    
public:
  ViewState state_;

  QEvaluationItem(const std::string& name, insight::cad::PostprocActionPtr smp, QoccViewerContext* context, 
		 const ViewState& state, QListWidget* view = 0);
  
  void reset(insight::cad::PostprocActionPtr smp);
  void wireframe();
  void shaded();
  void randomizeColor();
  void hide();
  void show();
  void updateDisplay();
  
 
public slots:
  void showContextMenu(const QPoint& gpos);
  
};


#endif // QEVALUATIONITEM_H
