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

#include "qvariableitem.h"

#include <string>
#include <QMenu>

#include "qoccviewercontext.h"
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;


QScalarVariableItem::QScalarVariableItem
(
    const QString& name,
    double value,
    QTreeWidgetItem* parent
)
: QModelTreeItem(name, parent),
  value_(value)
{
    setText(COL_VALUE, QString::number(v));
}




void QScalarVariableItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    QMenu myMenu;
    QAction* a;
    
    a=new QAction(name_, &myMenu);
    a->setDisabled(true);
    myMenu.addAction(a);
    
    myMenu.addSeparator();
    
    a=new QAction("Insert name", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(insertName()));
    myMenu.addAction(a);
    
    myMenu.exec(gpos);
}



Handle_AIS_InteractiveObject QVectorVariableItem::createAIS()
{
  TopoDS_Shape cP=BRepBuilderAPI_MakeVertex(to_Pnt(value_));
  return Handle_AIS_InteractiveObject(new AIS_Shape(cP));
}


QVectorVariableItem::QVectorVariableItem
(
 const QString& name,
 const arma::mat& value,
 QTreeWidgetItem* parent
)
: QDisplayableModelTreeItem(name, false, parent),
  value_(value)
{
    setText(COL_VALUE, QString::fromStdString
         (
             str(format("[%g, %g, %g]") % v(0) % v(1) % v(2))
         )
    );
}


void QVectorVariableItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    QMenu myMenu;
    QAction *a;
    
    a=new QAction(name_, &myMenu);
    a->setDisabled(true);
    myMenu.addAction(a);
    
    myMenu.addSeparator();
    
    a=new QAction("Insert name", &myMenu);
    connect(a, SIGNAL(triggered()),
            this, SLOT(insertName()));
    myMenu.addAction(a);

    myMenu.exec(gpos);
}
