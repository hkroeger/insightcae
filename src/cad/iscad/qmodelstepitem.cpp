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





QFeatureItem::QFeatureItem
(
  const std::string& name, 
  insight::cad::FeaturePtr smp, 
  QoccViewerContext* context, 
  const ViewState& state, 
  QTreeWidgetItem* parent,
  bool is_component
)
: QDisplayableModelTreeItem(name, context, state, parent),
  is_component_(is_component)
{
  setText(COL_NAME, name_);
  setCheckState(COL_VIS, state_.visible ? Qt::Checked : Qt::Unchecked);
  reset(smp);
}


// void QModelStepItem::run()
// {
//   rebuild();
// }
// 
void QFeatureItem::reset(insight::cad::FeaturePtr smp)
{
  smp_=smp;
  rebuild();
}

void QFeatureItem::rebuild()
{
  if (!ais_.IsNull()) context_->getContext()->Erase(ais_
#if (OCC_VERSION_MAJOR>=7)
                   , true
#endif                      
  );
  ais_=new AIS_Shape(*smp_);
//     Handle_Standard_Transient owner_container(new SolidModelTransient(smp));
  Handle_Standard_Transient owner_container(new PointerTransient(this));
  ais_->SetOwner(owner_container);
  context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
  updateDisplay();
}

void QFeatureItem::wireframe()
{
  state_.shading=0;
  updateDisplay();
}

void QFeatureItem::shaded()
{
  state_.shading=1;
  updateDisplay();
}

void QFeatureItem::onlyThisShaded()
{
//   qDebug()<<"all wireframe"<<endl;
  
  emit(setUniformDisplayMode(AIS_WireFrame));
  shaded();
}


void QFeatureItem::hide()
{
  setCheckState(COL_VIS, Qt::Unchecked);
  updateDisplay();
}

void QFeatureItem::show()
{
  setCheckState(COL_VIS, Qt::Checked);
  updateDisplay();
}


void QFeatureItem::randomizeColor()
{
  state_.randomizeColor();
  updateDisplay();
}

void QFeatureItem::updateDisplay()
{
  state_.visible = (checkState(COL_VIS)==Qt::Checked);
  
  if (state_.visible)
  {
    context_->getContext()->Display(ais_
#if (OCC_VERSION_MAJOR>=7)
                   , true
#endif                        
    );
    context_->getContext()->SetDisplayMode(ais_, state_.shading, Standard_True );
    context_->getContext()->SetColor(ais_, Quantity_Color(state_.r, state_.g, state_.b, Quantity_TOC_RGB), Standard_True );
  }
  else
  {
    context_->getContext()->Erase(ais_
#if (OCC_VERSION_MAJOR>=7)
                   , true
#endif                        
    );
  }
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

void QFeatureItem::insertName()
{
  emit insertParserStatementAtCursor(name_);
}

void QFeatureItem::resetDisplay()
{
    if (is_component_)
    {
        show();
    }
    else
    {
        hide();
    }
}

void QFeatureItem::showProperties()
{
  emit addEvaluation
  (
    "SolidProperties_"+name_.toStdString(), 
    insight::cad::PostprocActionPtr(new insight::cad::SolidProperties(smp_)),
    true
  );
}

void QFeatureItem::setResolution()
{
  bool ok;
  double res=QInputDialog::getDouble(treeWidget(), "Set Resolution", "Resolution:", 0.001, 1e-7, 0.1, 7, &ok);
  if (ok)
  {
    context_->getContext()->SetDeviationCoefficient(ais_, res
#if (OCC_VERSION_MAJOR>=7)
                   , true
#endif                        
    );
  }
}

void QFeatureItem::jump()
{
    emit(jump_to(name_));
}

void QFeatureItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    QMenu myMenu;
    QAction *a;
    
    a=new QAction(name_, &myMenu);
    connect(a, SIGNAL(triggered()), this, SLOT(jump()));
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
