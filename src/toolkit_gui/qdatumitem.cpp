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
//#include "qoccviewercontext.h"
#include "datum.h"
#include "AIS_Plane.hxx"
#include "AIS_Axis.hxx"
#include "Geom_Axis1Placement.hxx"
#include "cadfeatures/modelfeature.h"

#include "occguitools.h"

using namespace insight::cad;

Handle_AIS_InteractiveObject QDatumItem::createAIS(
      AIS_InteractiveContext& context
      )
{
  return createAIS( smp_, context, gp_Trsf() );
}


Handle_AIS_InteractiveObject QDatumItem::createAIS(
    insight::cad::DatumPtr datum,
    AIS_InteractiveContext& context,
    const gp_Trsf& tr
    )
{
  auto label = name_.toStdString();

  datum->checkForBuildDuringAccess();

  if (auto dp = std::dynamic_pointer_cast<DatumPoint>(datum))
  {
    return //smp_->createAISRepr( context, label );
        buildMultipleConnectedInteractive(context,
          {
            Handle_AIS_InteractiveObject(
             new AIS_Shape( BRepBuilderAPI_MakeVertex(dp->point().Transformed(tr)) )
            ),
            Handle_AIS_InteractiveObject(new InteractiveText(
             boost::str(boost::format("PT:%s") % label), insight::Vector(dp->point().Transformed(tr).XYZ())
            ))
           });
  }
  else if (auto dp = std::dynamic_pointer_cast<DatumAxis>(datum))
  {
    return buildMultipleConnectedInteractive(context,
    {
     Handle_AIS_InteractiveObject(new AIS_Axis(Handle_Geom_Axis1Placement(
                                               new Geom_Axis1Placement(dp->axis().Transformed(tr))))),
     Handle_AIS_InteractiveObject(new InteractiveText
       (
         boost::str(boost::format("AX:%s") % label), insight::Vector(dp->point().Transformed(tr).XYZ())
       ))
    });
  }
  else if (auto dp = std::dynamic_pointer_cast<DatumPlaneData>(datum))
  {
    auto plt = dp->plane().Transformed(tr);
    Handle_AIS_Plane aplane(new AIS_Plane(
            Handle_Geom_Plane(new Geom_Plane(plt))
    ));
    aplane->SetCenter(plt.Location()); // will be displayed around origin otherwise

    return buildMultipleConnectedInteractive(context,
    {
      aplane,
      Handle_AIS_InteractiveObject(new InteractiveText
       (
         boost::str(boost::format("PL:%s") % label), insight::Vector(dp->point().Transformed(tr).XYZ())
       ))
    });
  }
  else if (auto dp = std::dynamic_pointer_cast<TransformedDatum>(datum))
  {
    return createAIS(dp->baseDatum(), context, dp->trsf()*tr);
  }
  else if (auto dp = std::dynamic_pointer_cast<ProvidedDatum>(datum))
  {
    return createAIS(dp->baseDatum(), context, tr);
  }
  else
  {
    throw insight::Exception("Not implemented: provide AIS_InteractiveObject presentation");
    return nullptr;
  }
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
    connect(a, &QAction::triggered, this, &QDatumItem::shaded);
    myMenu.addAction(a);
    
    a=new QAction("Wireframe", &myMenu);
    connect(a, &QAction::triggered, this, &QDatumItem::wireframe);
    myMenu.addAction(a);
    
    a=new QAction("Randomize Color", &myMenu);
    connect(a, &QAction::triggered, this, &QDatumItem::randomizeColor);
    myMenu.addAction(a);

    myMenu.exec(gpos);
}
