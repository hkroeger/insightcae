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

QVariableItem::QVariableItem(const std::string& name, arma::mat vv, QoccViewerContext* context, 
		const ViewState& state, QListWidget* view )
: QListWidgetItem
  (
   QString::fromStdString(name+" = ["+lexical_cast<string>(vv(0))+", "+lexical_cast<string>(vv(1))+", "+lexical_cast<string>(vv(2))+"]"), 
   view
  ),
  name_(QString::fromStdString(name)),
  context_(context),
  state_(state)
{
  setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
  reset(vv);
}

void QVariableItem::createAISShape()
{
//   TopoDS_Edge cP = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(value_),gp_Dir(0,0,1)), 1));
//   Handle_AIS_Shape aisP = new AIS_Shape(cP);
  TopoDS_Shape cP=BRepBuilderAPI_MakeVertex(to_Pnt(value_));
  Handle_AIS_Shape aisP(new AIS_Shape(cP));
  
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

void QVariableItem::reset(arma::mat val)
{
  value_=val;
  if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
  createAISShape();
//     Handle_Standard_Transient owner_container(new SolidModelTransient(smp));
//   context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
  updateDisplay();
}

void QVariableItem::updateDisplay()
{
  state_.visible = (checkState()==Qt::Checked);
  
  if (state_.visible)
  {
    context_->getContext()->Display(ais_);
//     context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
//     context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
  }
  else
  {
    context_->getContext()->Erase(ais_);
  }
}

void QVariableItem::insertName()
{
  emit insertParserStatementAtCursor(name_);
}

void QVariableItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    // for QAbstractScrollArea and derived classes you would use:
    // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

    QMenu myMenu;
    QAction *tit=new QAction(name_, &myMenu);
    tit->setDisabled(true);
    myMenu.addAction(tit);
    myMenu.addSeparator();
    myMenu.addAction("Insert name");
    // ...

    QAction* selectedItem = myMenu.exec(gpos);
    if (selectedItem)
    {
	if (selectedItem->text()=="Insert name") insertName();
    }
    else
    {
	// nothing was chosen
    }
}
