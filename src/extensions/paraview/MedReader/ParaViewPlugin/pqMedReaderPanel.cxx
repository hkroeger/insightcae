// Copyright (C) 2010-2012  CEA/DEN, EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

#include "pqMedReaderPanel.h"
#include "ui_MedReaderPanel.h"

#include "vtkProcessModule.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVSILInformation.h"
#include "vtkGraph.h"
#include "vtkSMPropertyHelper.h"
#include "vtkStringArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkMedReader.h"

#include "vtkMedUtilities.h"

#include "pqTreeWidgetItemObject.h"
#include "pqSMAdaptor.h"
#include "pqProxy.h"
#include "pqPropertyManager.h"
#include "pqSILModel.h"
#include "pqProxySILModel.h"
#include "pqTreeViewSelectionHelper.h"
#include "pqTreeWidgetSelectionHelper.h"
#include "pqPropertyLinks.h"

#include <QHeaderView>

class pqMedReaderPanel::pqUI: public QObject, public Ui::MedReaderPanel
{
public:
  pqUI(pqMedReaderPanel* p) :
    QObject(p)
  {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    this->SILUpdateStamp = -1;
  }

  ~pqUI()
  {
  }

  pqSILModel SILModel;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  pqPropertyLinks Links;
  QMap<QTreeWidgetItem*, QString> TreeItemToPropMap;
  pqProxySILModel* entityModel;
  pqProxySILModel* groupModel;
  int SILUpdateStamp;
};

pqMedReaderPanel::pqMedReaderPanel(vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = 0) :
  Superclass(smproxy, parentObject)
{
  cerr<<"pqMedReaderPanel::pqMedReaderPanel(vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = 0)"<<endl;

  this->UI = new pqUI(this);
  this->UI->setupUi(this);

  pqProxySILModel* proxyModel;

  // connect groups to groupsRoot
  proxyModel = new pqProxySILModel("GroupTree", &this->UI->SILModel);
  proxyModel->setSourceModel(&this->UI->SILModel);
  this->UI->Groups->setModel(proxyModel);
  this->UI->Groups->setHeaderHidden(true);

  this->UI->groupModel = new pqProxySILModel("Groups", &this->UI->SILModel);
  this->UI->groupModel->setSourceModel(&this->UI->SILModel);

  // connect cell types to "EntityRoot"
  proxyModel = new pqProxySILModel("EntityTree", &this->UI->SILModel);
  proxyModel->setSourceModel(&this->UI->SILModel);
  this->UI->Entity->setModel(proxyModel);
  this->UI->Entity->setHeaderHidden(true);

  this->UI->entityModel = new pqProxySILModel("Entity", &this->UI->SILModel);
  this->UI->entityModel->setSourceModel(&this->UI->SILModel);

  this->updateSIL();

  this->UI->Groups->header()->setStretchLastSection(true);
  this->UI->Entity->header()->setStretchLastSection(true);

  this->linkServerManagerProperties();

  QList<pqTreeWidget*> treeWidgets = this->findChildren<pqTreeWidget*> ();
  foreach (pqTreeWidget* tree, treeWidgets)
      {
      new pqTreeWidgetSelectionHelper(tree);
      }

  QList<pqTreeView*> treeViews = this->findChildren<pqTreeView*> ();
  foreach (pqTreeView* tree, treeViews)
      {
      new pqTreeViewSelectionHelper(tree);
      }

  this->connect(this->UI->groupModel, SIGNAL(valuesChanged()),
                this, SLOT(setModified()));
  this->connect(this->UI->entityModel, SIGNAL(valuesChanged()),
                this, SLOT(setModified()));
  this->connect(this->UI->TimeCombo, SIGNAL(currentIndexChanged(int)),
                this, SLOT(setModified()));
  this->connect(this->UI->GenerateVectors, SIGNAL(stateChanged(int)),
                this, SLOT(setModified()));

  this->UI->tabWidget->setCurrentIndex(0);

  this->UI->VTKConnect->Connect(this->proxy(),
      vtkCommand::UpdateInformationEvent, this, SLOT(updateSIL()));
}

pqMedReaderPanel::~pqMedReaderPanel()
{
}

void pqMedReaderPanel::addSelectionsToTreeWidget(const QString& prop,
    QTreeWidget* tree, PixmapType pix)
{
  cerr<<"void pqMedReaderPanel::addSelectionsToTreeWidget(const QString& prop, QTreeWidget* tree, PixmapType pix)"<<endl;

  vtkSMProperty* SMProperty = this->proxy()->GetProperty(prop.toStdString().c_str());
  QList<QVariant> SMPropertyDomain;
  SMPropertyDomain = pqSMAdaptor::getSelectionPropertyDomain(SMProperty);
  int j;
  for(j = 0; j < SMPropertyDomain.size(); j++)
    {
    QString varName = SMPropertyDomain[j].toString();
    this->addSelectionToTreeWidget(varName, varName, tree, pix, prop, j);
    }
}

void pqMedReaderPanel::addSelectionToTreeWidget(const QString& name,
    const QString& realName, QTreeWidget* tree, PixmapType pix,
    const QString& prop, int propIdx)
{
  cerr<<"void pqMedReaderPanel::addSelectionToTreeWidget(const QString& name, const QString& realName, QTreeWidget* tree, PixmapType pix, const QString& prop, int propIdx)"<<endl;

  static QPixmap pixmaps[] = {
    QPixmap(":/ParaViewResources/Icons/pqPointData16.png"),
    QPixmap(":/ParaViewResources/Icons/pqCellData16.png"),
    QPixmap(":/ParaViewResources/Icons/pqQuadratureData16.png"),
    QPixmap(":/ParaViewResources/Icons/pqElnoData16.png") };

  vtkSMProperty* SMProperty = this->proxy()->GetProperty(prop.toStdString().c_str());

  if(!SMProperty || !tree)
    {
    return;
    }

  QList<QString> strs;
  strs.append(name);
  pqTreeWidgetItemObject* item;
  item = new pqTreeWidgetItemObject(tree, strs);
  item->setData(0, Qt::ToolTipRole, name);
  if(pix >= 0)
    {
    item->setData(0, Qt::DecorationRole, pixmaps[pix]);
    }
  item->setData(0, Qt::UserRole, QString("%1 %2").arg((int) pix).arg(realName));
#warning ok to remove?
//  this->propertyManager()->registerLink(item, "checked",
//      SIGNAL(checkedStateChanged(bool)), this->proxy(), SMProperty, propIdx);

  this->UI->TreeItemToPropMap[item] = prop;
}

void pqMedReaderPanel::linkServerManagerProperties()
{
  this->UI->Links.addPropertyLink(this->UI->groupModel, "values",
      SIGNAL(valuesChanged()), this->proxy(), this->proxy()->GetProperty(
          "Groups"));

  this->UI->Links.addPropertyLink(this->UI->entityModel, "values",
      SIGNAL(valuesChanged()), this->proxy(), this->proxy()->GetProperty(
          "Entity"));

  this->UI->Links.addPropertyLink(this->UI->GenerateVectors, "checked",
      SIGNAL(toggled(bool)), this->proxy(), this->proxy()->GetProperty(
          "GenerateVectors"));

#warning ok to remove?
//    this->Superclass::linkServerManagerProperties();

  // do the point variables
  this->addSelectionsToTreeWidget("PointFieldsArrayStatus",
      this->UI->Variables, PM_POINT);
  // do the cell variables
  this->addSelectionsToTreeWidget("CellFieldsArrayStatus", this->UI->Variables,
      PM_CELL);
  // do the quadrature variables
  this->addSelectionsToTreeWidget("QuadratureFieldsArrayStatus",
      this->UI->Variables, PM_QUADRATURE);

  // do the Elno variables
  this->addSelectionsToTreeWidget("ElnoFieldsArrayStatus",
      this->UI->Variables, PM_ELNO);
  this->setupAnimationModeWidget();
}

void pqMedReaderPanel::setupAnimationModeWidget()
{
  cerr<<"void pqMedReaderPanel::setupAnimationModeWidget()"<<endl;

  this->UI->AnimationModeCombo->clear();
  QList<QVariant> modes = pqSMAdaptor::getEnumerationPropertyDomain(
      this->proxy()->GetProperty("AnimationMode"));
  for(int mode = 0; mode < modes.size(); mode++)
    {
    QString modeName = modes[mode].toString();
    this->UI->AnimationModeCombo->addItem(modeName);
    }

  this->UI->Links.addPropertyLink(this->UI->AnimationModeCombo, "currentIndex",
      SIGNAL(currentIndexChanged(int)), this->proxy(),
      this->proxy()->GetProperty("AnimationMode"));

  this->connect(this->UI->AnimationModeCombo,
      SIGNAL(currentIndexChanged(int)), this, SLOT(animationModeChanged(int)));

  this->UI->Links.addPropertyLink(this->UI->TimeCombo, "currentIndex",
      SIGNAL(currentIndexChanged(int)), this->proxy(),
      this->proxy()->GetProperty("TimeIndexForIterations"));

  this->addSelectionsToTreeWidget("FrequencyArrayStatus",
      this->UI->Modes, PM_NONE);

  vtkSMPropertyHelper helper(this->proxy(), "AnimationMode");
  int mode = helper.GetAsInt(0);
  this->animationModeChanged(mode);
  this->updateAvailableTimes();
}

void pqMedReaderPanel::animationModeChanged(int mode)
{
  cerr<<"void pqMedReaderPanel::animationModeChanged(int mode)"<<endl;

  if(mode == vtkMedReader::Default || mode == vtkMedReader::PhysicalTime)
    {
    this->UI->TimeCombo->hide();
    this->UI->TimeLabel->hide();
    this->UI->Modes->hide();
    }
  else if(mode == vtkMedReader::Iteration)
    {
    this->UI->TimeCombo->show();
    this->UI->TimeLabel->show();
    this->UI->Modes->hide();
    }
  else // vtkMedReader::Modes
    {
    this->UI->TimeCombo->hide();
    this->UI->TimeLabel->hide();
    this->UI->Modes->show();
    }
  vtkSMPropertyHelper(this->proxy(), "AnimationMode").Set(mode);
  this->proxy()->UpdateVTKObjects();
#warning ok to remove?
//  this->setModified();
}

void pqMedReaderPanel::updateAvailableTimes()
{
  cerr<<"void pqMedReaderPanel::updateAvailableTimes()"<<endl;

  vtkSMDoubleVectorProperty* prop = vtkSMDoubleVectorProperty::SafeDownCast(
      this->proxy()->GetProperty("AvailableTimes"));

  this->proxy()->UpdatePropertyInformation(prop);

  //prop->GetInformationHelper()->UpdateProperty(
  //    vtkProcessModuleConnectionManager::GetRootServerConnectionID(),
  //    vtkProcessModule::DATA_SERVER,
  //    this->proxy()->GetID(),
  //    prop);

  this->UI->TimeCombo->clear();
  double *aux = prop->GetElements();

  for(int tid = 0; tid < prop->GetNumberOfElements(); tid++)
    {
    this->UI->TimeCombo->addItem( QString::number(aux[tid]) );
    }

}

void pqMedReaderPanel::updateSIL()
{
  cerr<<"void pqMedReaderPanel::updateSIL()"<<endl;

#warning ok to remove?
//  vtkSMProxy* reader = this->referenceProxy()->getProxy();
//  reader->UpdatePropertyInformation(reader->GetProperty("SILUpdateStamp"));

//  int stamp = vtkSMPropertyHelper(reader, "SILUpdateStamp").GetAsInt();
//  if (stamp != this->UI->SILUpdateStamp)
//    {
//    this->UI->SILUpdateStamp = stamp;
//    vtkPVSILInformation* info = vtkPVSILInformation::New();
//    reader->GatherInformation(info);
//    this->UI->SILModel.update(info->GetSIL());

//    this->UI->Groups->expandAll();
//    this->UI->Entity->expandAll();

//    info->Delete();
//    }

  /*
  this->proxy()->UpdatePropertyInformation(
      this->proxy()->GetProperty("SILUpdateStamp"));

  int stamp = vtkSMPropertyHelper(this->proxy(), "SILUpdateStamp").GetAsInt();
  if(stamp != this->UI->SILUpdateStamp)
    {
    this->UI->SILUpdateStamp = stamp;

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkPVSILInformation* info = vtkPVSILInformation::New();
    pm->GatherInformation(this->proxy()->GetConnectionID(),
        vtkProcessModule::DATA_SERVER, info, this->proxy()->GetID());

    this->UI->SILModel.update(info->GetSIL());

    this->UI->Groups->expandAll();
    this->UI->Entity->expandAll();

    info->Delete();
    }*/
}
