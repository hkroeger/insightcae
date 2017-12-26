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

#include <QMenu>
#include "qoccviewercontext.h"

QEvaluationItem::QEvaluationItem(const std::string& name, insight::cad::PostprocActionPtr smp, QoccViewerContext* context, 
		const ViewState& state, QTreeWidgetItem* parent)
: QDisplayableModelTreeItem(name, context, state, parent)
{
    setText(COL_NAME, name_);
  setCheckState(COL_VIS, state_.visible ? Qt::Checked : Qt::Unchecked);
  reset(smp);
}

void QEvaluationItem::reset(insight::cad::PostprocActionPtr smp)
{
  smp_=smp;
  if (!ais_.IsNull()) context_->getContext()->Erase(ais_
#if (OCC_VERSION_MAJOR>=7)
                   , false
#endif                      
  );
  ais_=smp_->createAISRepr(context_->getContext());
  if (!ais_.IsNull()) 
  {
    context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
    context_->getContext()->SetColor( ais_, Quantity_NOC_BLACK, false );
  }
  updateDisplay();
}

void QEvaluationItem::wireframe()
{
  state_.shading=0;
  updateDisplay();
}

void QEvaluationItem::shaded()
{
  state_.shading=1;
  updateDisplay();
}

void QEvaluationItem::randomizeColor()
{
  state_.randomizeColor();
  updateDisplay();
}

void QEvaluationItem::hide()
{
  setCheckState(COL_VIS, Qt::Unchecked);
  updateDisplay();
}

void QEvaluationItem::show()
{
  setCheckState(COL_VIS, Qt::Checked);
  updateDisplay();
}

void QEvaluationItem::updateDisplay()
{
  state_.visible = (checkState(COL_VIS)==Qt::Checked);
  
  if (!ais_.IsNull())
  {
    if (state_.visible)
    {
      context_->getContext()->Display(ais_
#if (OCC_VERSION_MAJOR>=7)
                   , false
#endif                          
      );
      context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
      context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
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

