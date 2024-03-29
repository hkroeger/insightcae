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

#include "qevaluationitem.h"
#include "postprocactionvisualizer.h"

#include <QMenu>
//#include "qoccviewercontext.h"

using namespace insight::cad;

Handle_AIS_InteractiveObject QEvaluationItem::createAIS(AIS_InteractiveContext&)
{
  insight::CurrentExceptionContext ec("creating AIS representation of CAD feature");
  try {
    return postProcActionVisualizers.createAISRepr(smp_);
  }
  catch (Standard_Failure& e)
  {
    throw insight::Exception(e.GetMessageString()?e.GetMessageString():"(no error message)");
  }
}


QEvaluationItem::QEvaluationItem
(
    const QString& name,
    insight::cad::PostprocActionPtr smp,
    QTreeWidgetItem* parent,
    bool visible
)
: QDisplayableModelTreeItem(name, visible, AIS_WireFrame, parent),
  smp_(smp)
{
}


void QEvaluationItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    // for QAbstractScrollArea and derived classes you would use:
    // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

    QMenu myMenu;
    QAction *a;
    
    a=new QAction(name_, &myMenu);
    a->setDisabled(true);
    myMenu.addAction(a);
    
    myMenu.addSeparator();
    
    a=new QAction("Shaded", &myMenu);
    connect(a, &QAction::triggered, this, &QEvaluationItem::shaded);
    myMenu.addAction(a);
    
    a=new QAction("Wireframe", &myMenu);
    connect(a, &QAction::triggered, this, &QEvaluationItem::wireframe);
    myMenu.addAction(a);
    
    a=new QAction("Randomize Color", &myMenu);
    connect(a, &QAction::triggered, this, &QEvaluationItem::randomizeColor);
    myMenu.addAction(a);

    myMenu.exec(gpos);
}

