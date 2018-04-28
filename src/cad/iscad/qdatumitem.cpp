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

#include "qdatumitem.h"

#include <QMenu>
#include "qoccviewercontext.h"
#include "datum.h"
#include "AIS_Plane.hxx"
#include "modelfeature.h"

Handle_AIS_InteractiveObject QDatumItem::createAIS(AIS_InteractiveContext& context)
{
  return smp_->createAISRepr( context, name_.toStdString() );
}

QDatumItem::QDatumItem(const QString& name, insight::cad::DatumPtr smp, QTreeWidgetItem* parent)
: QDisplayableModelTreeItem(name, true, AIS_WireFrame, parent),
  smp_(smp)
{}


void QDatumItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    QMenu myMenu;
    QAction *a;
    
    a=new QAction(name_, &myMenu);
    a->setDisabled(true);
    myMenu.addAction(a);
    
    myMenu.addSeparator();
    
    a=new QAction("Shaded", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(shaded()));
    myMenu.addAction(a);
    
    a=new QAction("Wireframe", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(wireframe()));
    myMenu.addAction(a);
    
    a=new QAction("Randomize Color", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(randomizeColor()));
    myMenu.addAction(a);

    myMenu.exec(gpos);
}
