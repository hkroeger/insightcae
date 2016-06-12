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


QDatumItem::QDatumItem(const std::string& name, insight::cad::DatumPtr smp, QoccViewerContext* context, 
		const ViewState& state, QListWidget* view = 0)
: QListWidgetItem(QString::fromStdString(name), view),
  context_(context),
  state_(state)
{
  setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
  reset(smp);
}

void QDatumItem::reset(insight::cad::DatumPtr smp)
{
  smp_=smp;
  if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
  ais_=smp_->createAISRepr();
  context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
  updateDisplay();
}

void QDatumItem::wireframe()
{
  state_.shading=0;
  updateDisplay();
}

void QDatumItem::shaded()
{
  state_.shading=1;
  updateDisplay();
}

void QDatumItem::randomizeColor()
{
  state_.randomizeColor();
  updateDisplay();
}

void QDatumItem::updateDisplay()
{
  state_.visible = (checkState()==Qt::Checked);
  
  if (state_.visible)
  {
    context_->getContext()->Display(ais_);
    context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
    context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
  }
  else
  {
    context_->getContext()->Erase(ais_);
  }
}



void QDatumItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    // for QAbstractScrollArea and derived classes you would use:
    // QPoint globalPos = myWidget->viewport()->mapToGlobal(pos);

    QMenu myMenu;
    myMenu.addAction("Shaded");
    myMenu.addAction("Wireframe");
    myMenu.addAction("Randomize Color");
//       myMenu.addAction("Export...");
    // ...

    QAction* selectedItem = myMenu.exec(gpos);
    if (selectedItem)
    {
	if (selectedItem->text()=="Shaded") shaded();
	if (selectedItem->text()=="Wireframe") wireframe();
	if (selectedItem->text()=="Randomize Color") randomizeColor();
// 	  if (selectedItem->text()=="Export...") exportShape();
    }
    else
    {
	// nothing was chosen
    }
}
