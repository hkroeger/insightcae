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



ModelStepItemAdder::ModelStepItemAdder
(
  ISCADMainWindow* mw,
  QModelStepItem* msi
)
: QThread(), mw_(mw), msi_(msi)
{}




void ModelStepItemAdder::run()
{
  msi_->rebuild();
  
  connect
  (
    msi_, SIGNAL(insertParserStatementAtCursor(const QString&)),
    mw_->editor_, SLOT(insertPlainText(const QString&))
  );
  connect
  (
    msi_, SIGNAL(jump_to(const QString&)),
    mw_, SLOT(jump_to(const QString&))
  );
  connect
  (
    msi_, SIGNAL(setUniformDisplayMode(const AIS_DisplayMode)),
    mw_, SLOT(setUniformDisplayMode(const AIS_DisplayMode))
  );
  connect
  (
    msi_, SIGNAL(addEvaluation(std::string, insight::cad::PostprocActionPtr, bool)),
    mw_, SLOT(addEvaluation(std::string, insight::cad::PostprocActionPtr, bool))
  );
  mw_->modelsteplist_->addItem(msi_);
}




QModelStepItem::QModelStepItem
(
  const std::string& name, 
  insight::cad::FeaturePtr smp, 
  QoccViewerContext* context, 
  const ViewState& state, 
  QListWidget* view,
  bool is_component
)
: QListWidgetItem(QString::fromStdString(name), view),
  name_(QString::fromStdString(name)),
  context_(context),
  state_(state),
  is_component_(is_component)
{
  setCheckState(state_.visible ? Qt::Checked : Qt::Unchecked);
  reset(smp);
//   smp_=smp;
}


// void QModelStepItem::run()
// {
//   rebuild();
// }
// 
void QModelStepItem::reset(insight::cad::FeaturePtr smp)
{
  smp_=smp;
  rebuild();
}

void QModelStepItem::rebuild()
{
  if (!ais_.IsNull()) context_->getContext()->Erase(ais_);
  ais_=new AIS_Shape(*smp_);
//     Handle_Standard_Transient owner_container(new SolidModelTransient(smp));
  Handle_Standard_Transient owner_container(new PointerTransient(this));
  ais_->SetOwner(owner_container);
  context_->getContext()->SetMaterial( ais_, Graphic3d_NOM_SATIN, false );
  updateDisplay();
}

void QModelStepItem::wireframe()
{
  state_.shading=0;
  updateDisplay();
}

void QModelStepItem::shaded()
{
  state_.shading=1;
  updateDisplay();
}

void QModelStepItem::onlyThisShaded()
{
//   qDebug()<<"all wireframe"<<endl;
  
  emit(setUniformDisplayMode(AIS_WireFrame));
  shaded();
}


void QModelStepItem::hide()
{
  setCheckState(Qt::Unchecked);
  updateDisplay();
}

void QModelStepItem::show()
{
  setCheckState(Qt::Checked);
  updateDisplay();
}


void QModelStepItem::randomizeColor()
{
  state_.randomizeColor();
  updateDisplay();
}

void QModelStepItem::updateDisplay()
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

void QModelStepItem::exportShape()
{
  QString fn=QFileDialog::getSaveFileName
  (
    listWidget(), 
    "Export file name", 
    "", "BREP file (*,brep);;ASCII STL file (*.stl);;Binary STL file (*.stlb);;IGES file (*.igs);;STEP file (*.stp)"
  );
  if (!fn.isEmpty()) smp_->saveAs(qPrintable(fn));
}

void QModelStepItem::insertName()
{
  emit insertParserStatementAtCursor(name_);
}

void QModelStepItem::resetDisplay()
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

void QModelStepItem::showProperties()
{
  emit addEvaluation
  (
    "SolidProperties_"+name_.toStdString(), 
    insight::cad::PostprocActionPtr(new insight::cad::SolidProperties(smp_)),
    true
  );
}

void QModelStepItem::setResolution()
{
  bool ok;
  double res=QInputDialog::getDouble(listWidget(), "Set Resolution", "Resolution:", 0.001, 1e-7, 0.1, 7, &ok);
  if (ok)
  {
    context_->getContext()->SetDeviationCoefficient(ais_, res);
  }
}


void QModelStepItem::showContextMenu(const QPoint& gpos) // this is a slot
{
    QMenu myMenu;
    QAction *tit=new QAction(name_, &myMenu);
//     tit->setDisabled(true);
    myMenu.addAction(tit);
    myMenu.addSeparator();
    myMenu.addAction("Insert name");
    myMenu.addSeparator();
    myMenu.addAction("Show");
    myMenu.addAction("Hide");
    myMenu.addAction("Shaded");
    myMenu.addAction("Only this shaded");
    myMenu.addAction("Wireframe");
    myMenu.addAction("Randomize Color");
    myMenu.addAction("Show Properties");
    myMenu.addAction("Set Resolution...");
    myMenu.addAction("Export...");

    QAction* selectedItem = myMenu.exec(gpos);
    if (selectedItem)
    {
	if (selectedItem->text()==name_) emit(jump_to(name_));
	if (selectedItem->text()=="Show") show();
	if (selectedItem->text()=="Hide") hide();
	if (selectedItem->text()=="Shaded") shaded();
	if (selectedItem->text()=="Wireframe") wireframe();
	if (selectedItem->text()=="Only this shaded") onlyThisShaded();
	if (selectedItem->text()=="Randomize Color") randomizeColor();
	if (selectedItem->text()=="Insert name") insertName();
	if (selectedItem->text()=="Show Properties") showProperties();
	if (selectedItem->text()=="Set Resolution...") setResolution();
	if (selectedItem->text()=="Export...") exportShape();
    }
    else
    {
    }
}
