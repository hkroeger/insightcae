/*
 *
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

#include <QFileDialog>
#include <QMenu>
#include <QAction>

#include "base/qt5_helper.h"

#ifndef Q_MOC_RUN
#include "qmodelstepitem.h"
#include "pointertransient.h"
#include "cadpostprocactions.h"
#endif

Handle_AIS_InteractiveObject QFeatureItem::createAIS(AIS_InteractiveContext&)
{
  Handle_AIS_InteractiveObject ais( /*new AIS_Shape(*smp_)*/ smp_->buildVisualization() );

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
    
    a=new QAction(name_+": Jump to Def.", &myMenu);
    connect(a, &QAction::triggered, this, &QFeatureItem::jumpToName);
    myMenu.addAction(a);

    a=new QAction("Insert name", &myMenu);
    connect(a, &QAction::triggered, this, &QFeatureItem::insertName);
    myMenu.addAction(a);

    myMenu.addSeparator();

    bool someSubMenu=false, someHoverDisplay=false;
    if (smp_->getDatumScalars().size()>0)
    {
      someSubMenu=true;
      QMenu *sm = new QMenu("Scalar Symbols");
      myMenu.addMenu(sm);
      for (auto i: smp_->getDatumScalars())
      {
        QAction *a = new QAction( QString::fromStdString(
                                    boost::str(boost::format("%s = %g") % i.first % i.second)
                                    ) );
        sm->addAction(a);
        connect(a, &QAction::triggered, [=]() {
            insertIntoNotebook( name_+"$"+QString::fromStdString(i.first) );
          });
      }
    }
    if (smp_->getDatumPoints().size()>0)
    {
      someSubMenu=true;
      QMenu *sm = new QMenu("Point Symbols");
      myMenu.addMenu(sm);
      for (auto i: smp_->getDatumPoints())
      {
        QAction *a = new QAction( QString::fromStdString(
                                    boost::str(boost::format("%s = [%g %g %g]") % i.first % i.second(0) % i.second(1) % i.second(2))
                                    ) );
        sm->addAction(a);
        connect(a, &QAction::triggered, [=]() {
            insertIntoNotebook( name_+"@"+QString::fromStdString(i.first) );
          });
        connect(a, &QAction::hovered,
                [=]() {
          gp_Pnt p=to_Pnt(i.second);
          focus(Handle_AIS_InteractiveObject(new AIS_Shape( BRepBuilderAPI_MakeVertex(p))));
        });
        someHoverDisplay=true;
      }
    }
    if (smp_->getDatumVectors().size()>0)
    {
      someSubMenu=true;
      QMenu *sm = new QMenu("Vector Symbols");
      myMenu.addMenu(sm);
      for (auto i: smp_->getDatumVectors())
      {
        QAction *a = new QAction( QString::fromStdString(
                                    boost::str(boost::format("%s = [%g %g %g]") % i.first % i.second(0) % i.second(1) % i.second(2))
                                    ) );
        sm->addAction(a);
        connect(a, &QAction::triggered, [=]() {
            insertIntoNotebook( name_+"^"+QString::fromStdString(i.first) );
          });
      }
    }
    if (smp_->providedSubshapes().size()>0)
    {
      someSubMenu=true;
      QMenu *sm = new QMenu("Subshapes");
      myMenu.addMenu(sm);
      for (auto i: smp_->providedSubshapes())
      {
        QAction *a = new QAction( QString::fromStdString(i.first) );
        sm->addAction(a);
        connect(a, &QAction::triggered, [=]() {
            insertIntoNotebook( name_+"."+QString::fromStdString(i.first) );
          });
        connect(a, &QAction::hovered,
                [=]() {
          focus(i.second->buildVisualization());
        });
        someHoverDisplay=true;
      }
    }

    if (someHoverDisplay)
    {
      connect(&myMenu, &QMenu::aboutToHide,
            this, &QFeatureItem::unfocus);
    }

    if (someSubMenu) myMenu.addSeparator();
    
    a=new QAction("Show", &myMenu);
    connect(a, &QAction::triggered, this, QOverload<>::of(&QFeatureItem::show));
    myMenu.addAction(a);

    a=new QAction("Hide", &myMenu);
    connect(a, &QAction::triggered, this, QOverload<>::of(&QFeatureItem::hide));
    myMenu.addAction(a);
    
    a=new QAction("Shaded", &myMenu);
    connect(a, &QAction::triggered, this, &QFeatureItem::shaded);
    myMenu.addAction(a);
    
    a=new QAction("Only this shaded", &myMenu);
    connect(a, &QAction::triggered, this, &QFeatureItem::onlyThisShaded);
    myMenu.addAction(a);
    
    a=new QAction("Wireframe", &myMenu);
    connect(a, &QAction::triggered, this, &QFeatureItem::wireframe);
    myMenu.addAction(a);
    
    a=new QAction("Randomize Color", &myMenu);
    connect(a, &QAction::triggered, this, &QFeatureItem::randomizeColor);
    myMenu.addAction(a);

    myMenu.addSeparator();

    a=new QAction("Show Properties", &myMenu);
    connect(a, &QAction::triggered, this, &QFeatureItem::showProperties);
    myMenu.addAction(a);
    
    a=new QAction("Set Resolution...", &myMenu);
    connect(a, &QAction::triggered, this, QOverload<>::of(&QFeatureItem::setResolution));
    myMenu.addAction(a);
    
    a=new QAction("Export...", &myMenu);
    connect(a, &QAction::triggered, this, &QFeatureItem::exportShape);
    myMenu.addAction(a);

    myMenu.exec(gpos);
}
