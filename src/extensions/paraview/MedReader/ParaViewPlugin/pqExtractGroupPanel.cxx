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

#include "pqExtractGroupPanel.h"
#include "ui_ExtractGroupPanel.h"

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

#include "vtkMedUtilities.h"

#include "pqTreeWidget.h"
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

class pqExtractGroupPanel::pqUI : public QObject, public Ui::ExtractGroupPanel
{
public:
  pqUI(pqExtractGroupPanel* p) :
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
  pqProxySILModel* entityModel;
  pqProxySILModel* groupModel;
  int SILUpdateStamp;
};

pqExtractGroupPanel::pqExtractGroupPanel(vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject) :
  Superclass(smproxy, parentObject)
{
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

  this->UI->tabWidget->setCurrentIndex(0);

  this->UI->VTKConnect->Connect(this->proxy(),
      vtkCommand::UpdateInformationEvent, this, SLOT(updateSIL()));

}

pqExtractGroupPanel::~pqExtractGroupPanel()
{
}

void pqExtractGroupPanel::linkServerManagerProperties()
{
  this->UI->Links.addPropertyLink(this->UI->groupModel, "values",
      SIGNAL(valuesChanged()), this->proxy(), this->proxy()->GetProperty(
          "Groups"));

  this->UI->Links.addPropertyLink(this->UI->entityModel, "values",
      SIGNAL(valuesChanged()), this->proxy(), this->proxy()->GetProperty(
          "Entity"));

#warning ok to remove?
  //this->Superclass::linkServerManagerProperties();

}

void pqExtractGroupPanel::updateSIL()
{

  vtkSMProxy* reader = proxy();
  reader->UpdatePropertyInformation(reader->GetProperty("SILUpdateStamp"));

  int stamp = vtkSMPropertyHelper(reader, "SILUpdateStamp").GetAsInt();
  if (stamp != this->UI->SILUpdateStamp)
    {
    this->UI->SILUpdateStamp = stamp;
    vtkPVSILInformation* info = vtkPVSILInformation::New();
    reader->GatherInformation(info);
#warning ok to remove?
    //this->UI->SILModel.update(info->GetSIL());

    this->UI->Groups->expandAll();
    this->UI->Entity->expandAll();

    info->Delete();
    }

}
