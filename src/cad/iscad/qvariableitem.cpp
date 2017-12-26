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

QScalarVariableItem::QScalarVariableItem(const std::string& name, double v, QTreeWidgetItem* parent)
: QModelTreeItem(name, parent)
{
    setText(COL_NAME, name_);
    setText(COL_VALUE, QString::number(v));
    reset(v);
}

void QScalarVariableItem::reset(double v)
{
    value_=v;
}


void QScalarVariableItem::insertName()
{
  emit insertParserStatementAtCursor(name_);
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




QVectorVariableItem::QVectorVariableItem
(
 const std::string& name, 
 arma::mat v, 
 QoccViewerContext* context, 
 const ViewState& state, 
 QTreeWidgetItem* parent
)
: QDisplayableModelTreeItem(name, context, state, parent)
{
    setText(COL_NAME, name_);
    setText(COL_VALUE, QString::fromStdString
         (
             str(format("[%g, %g, %g]") % v(0) % v(1) % v(2))
         )
    );
    setCheckState(COL_VIS, state_.visible ? Qt::Checked : Qt::Unchecked);
    reset(v);
}

void QVectorVariableItem::createAISShape()
{
//   TopoDS_Edge cP = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(value_),gp_Dir(0,0,1)), 1));
//   Handle_AIS_Shape aisP = new AIS_Shape(cP);
  TopoDS_Shape cP=BRepBuilderAPI_MakeVertex(to_Pnt(value_));
  Handle_AIS_Shape aisP(new AIS_Shape(cP));

  context_->getContext()->Load(aisP);
  context_->getContext()->SetColor(aisP, Quantity_Color(0, 0, 0, Quantity_TOC_RGB), false/*, Standard_True*/ );
  
//   Handle_AIS_InteractiveObject aisPLabel (createArrow(
//     cP, name_.toStdString())
//   );
// 
//   std::auto_ptr<AIS_MultipleConnectedInteractive> ais(new AIS_MultipleConnectedInteractive());
// 
//   Handle_Standard_Transient owner_container(new PointerTransient(this));
//   aisP->SetOwner(owner_container);
//   aisPLabel->SetOwner(owner_container);
//   ais->Connect(aisP);
//   ais->Connect(aisPLabel);
//   ais_=ais.release();
  ais_=aisP;
}

void QVectorVariableItem::reset(arma::mat val)
{
  value_=val;
  if (!ais_.IsNull()) context_->getContext()->Erase(ais_
#if (OCC_VERSION_MAJOR>=7)
                   , false
#endif                
    );
  createAISShape();
//     Handle_Standard_Transient owner_container(new SolidModelTransient(smp));
//   context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
  updateDisplay();
}

void QVectorVariableItem::updateDisplay()
{
  state_.visible = (checkState(COL_VIS)==Qt::Checked);
  
  if (state_.visible)
  {
    context_->getContext()->Display(ais_
#if (OCC_VERSION_MAJOR>=7)
                   , false
#endif                        
    );
//     context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
//     context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
  }
  else
  {
    context_->getContext()->Erase(ais_
#if (OCC_VERSION_MAJOR>=7)
                   , false
#endif                        
    );
  }
}

void QVectorVariableItem::insertName()
{
  emit insertParserStatementAtCursor(name_);
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
    connect(a, SIGNAL(triggered()), this, SLOT(insertName()));
    myMenu.addAction(a);

    myMenu.exec(gpos);
}
