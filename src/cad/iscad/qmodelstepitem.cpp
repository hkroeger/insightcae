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

#include "qmodelstepitem.h"

#include "iscadmainwindow.h"
#include "cadpostprocactions.h"



Handle_AIS_InteractiveObject QFeatureItem::createAIS(AIS_InteractiveContext&)
{
  Handle_AIS_InteractiveObject ais( new AIS_Shape(*smp_) );

  Handle_Standard_Transient owner_container(new PointerTransient(this));
  ais->SetOwner(owner_container);

  return ais;
}



QFeatureItem::QFeatureItem
(
  const QString& name,
  insight::cad::FeaturePtr smp, 
  bool visible,
  QTreeWidgetItem* parent,
  bool is_component
)
: QDisplayableModelTreeItem(name, visible, is_component ? AIS_Shaded : AIS_WireFrame, parent),
  smp_(smp),
  is_component_(is_component)
{
}


void QFeatureItem::exportShape()
{
  QString fn=QFileDialog::getSaveFileName
  (
    treeWidget(), 
    "Export file name", 
    "", "BREP file (*.brep);;ASCII STL file (*.stl);;Binary STL file (*.stlb);;IGES file (*.igs);;STEP file (*.stp)"
  );
  if (!fn.isEmpty()) smp_->saveAs(qPrintable(fn));
}



void QFeatureItem::showProperties()
{
  emit addEvaluation
  (
    "SolidProperties_"+name_,
    insight::cad::PostprocActionPtr(new insight::cad::SolidProperties(smp_)),
    true
  );
}



void QFeatureItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    QMenu myMenu;
    QAction *a;
    
    a=new QAction(name_, &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(jumpToName()));
    myMenu.addAction(a);
    
    myMenu.addSeparator();
    
    a=new QAction("Insert name", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(insertName()));
    myMenu.addAction(a);
    
    myMenu.addSeparator();

    a=new QAction("Show", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(show()));
    myMenu.addAction(a);

    a=new QAction("Hide", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(hide()));
    myMenu.addAction(a);
    
    a=new QAction("Shaded", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(shaded()));
    myMenu.addAction(a);
    
    a=new QAction("Only this shaded", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(onlyThisShaded()));
    myMenu.addAction(a);
    
    a=new QAction("Wireframe", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(wireframe()));
    myMenu.addAction(a);
    
    a=new QAction("Randomize Color", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(randomizeColor()));
    myMenu.addAction(a);

    a=new QAction("Show Properties", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(showProperties()));
    myMenu.addAction(a);
    
    a=new QAction("Set Resolution...", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(setResolution()));
    myMenu.addAction(a);
    
    a=new QAction("Export...", &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(exportShape()));
    myMenu.addAction(a);

    myMenu.exec(gpos);
}
