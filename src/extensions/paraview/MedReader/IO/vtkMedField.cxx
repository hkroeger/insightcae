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

#include "vtkMedField.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkMedFieldOverEntity.h"
#include "vtkMedFieldStep.h"
#include "vtkMedUtilities.h"
#include "vtkMedFieldOnProfile.h"
#include "vtkMedInterpolation.h"
#include "vtkMedFile.h"

#include <string>
#include <map>
using namespace std;

vtkCxxGetObjectVectorMacro(vtkMedField, Interpolation, vtkMedInterpolation);
vtkCxxSetObjectVectorMacro(vtkMedField, Interpolation, vtkMedInterpolation);

vtkCxxSetObjectMacro(vtkMedField, ParentFile, vtkMedFile);

vtkStandardNewMacro(vtkMedField)

vtkMedField::vtkMedField()
{
  this->NumberOfComponent = -1;
  this->DataType = MED_FLOAT64;
  this->Name = NULL;
  this->MeshName = NULL;
  this->TimeUnit = NULL;
  this->FieldStep = new vtkMedComputeStepMap<vtkMedFieldStep> ();
  this->Unit = vtkStringArray::New();
  this->ComponentName = vtkStringArray::New();
  this->Interpolation = new vtkObjectVector<vtkMedInterpolation> ();
  this->MedIterator = -1;
  this->FieldType = UnknownFieldType;
  this->ParentFile = NULL;
  this->Local = 1;
}

vtkMedField::~vtkMedField()
{
  this->SetName(NULL);
  this->SetMeshName(NULL);
  this->SetTimeUnit(NULL);
  delete this->FieldStep;
  this->Unit->Delete();
  this->ComponentName->Delete();
}

void vtkMedField::ComputeFieldType()
{
  this->FieldType = UnknownFieldType;

  // look for the med_entity_type
  // on which this field is.
  for(int sid = 0; sid < this->GetNumberOfFieldStep(); sid++)
    {
    vtkMedFieldStep* step = this->GetFieldStep(sid);
  
    for(int eid = 0; eid < step->GetNumberOfFieldOverEntity(); eid++)
      {
      vtkMedFieldOverEntity* fieldOverEntity = step->GetFieldOverEntity(eid);
      med_entity_type type = fieldOverEntity->GetEntity().EntityType;

      if (type == MED_NODE)
        {
        this->FieldType |= PointField;
        }
      else if(type == MED_NODE_ELEMENT )
        {
        this->FieldType |= ElnoField;
        }
      else
        {
        for(int pid=0; pid<fieldOverEntity->GetNumberOfFieldOnProfile(); pid++)
          {
          vtkMedFieldOnProfile* fop = fieldOverEntity->GetFieldOnProfile(pid);
          const char* locname = fop->GetLocalizationName();
          if(strcmp(locname, MED_GAUSS_ELNO) == 0 )
            {
            this->FieldType = ElnoField;
            }
          else if(strcmp(locname, MED_NO_LOCALIZATION) != 0 )
            {
            this->FieldType |= QuadratureField;
            }
          else
            {
            this->FieldType |= CellField;
            }
          }
        }
      }
    }
    
  if(this->FieldType == UnknownFieldType) 
    this->FieldType = PointField;
}

int vtkMedField::HasManyFieldTypes()
{
  int numberOfTypes = 0;
  numberOfTypes += (this->FieldType & vtkMedField::PointField) != 0;
  numberOfTypes += (this->FieldType & vtkMedField::CellField) != 0;
  numberOfTypes += (this->FieldType & vtkMedField::QuadratureField) != 0;
  numberOfTypes += (this->FieldType & vtkMedField::ElnoField) != 0;

  return numberOfTypes > 1;
}

int vtkMedField::GetFirstType()
{
  if((this->FieldType & vtkMedField::PointField) != 0)
    return vtkMedField::PointField;

  if((this->FieldType & vtkMedField::CellField) != 0)
    return vtkMedField::CellField;

  if((this->FieldType & vtkMedField::QuadratureField) != 0)
    return vtkMedField::QuadratureField;

  if((this->FieldType & vtkMedField::ElnoField) != 0)
    return vtkMedField::ElnoField;

  return vtkMedField::UnknownFieldType;
}

void  vtkMedField::ExtractFieldType(vtkMedField* otherfield, int type)
{
  this->SetName(otherfield->GetName());
  this->SetLocal(otherfield->GetLocal());
  this->SetMedIterator(otherfield->GetMedIterator());
  this->SetDataType(otherfield->GetDataType());
  this->SetMeshName(otherfield->GetMeshName());
  this->SetTimeUnit(otherfield->GetTimeUnit());
  this->SetParentFile(otherfield->GetParentFile());

  this->SetNumberOfComponent(otherfield->GetNumberOfComponent());
  for(int i=0; i< this->GetNumberOfComponent(); i++)
    {
    this->GetComponentName()->SetValue(i, otherfield->
                                       GetComponentName()->GetValue(i));
    }

  this->AllocateNumberOfInterpolation(otherfield->GetNumberOfInterpolation());
  for(int i=0; i<this->GetNumberOfInterpolation(); i++)
    {
    this->SetInterpolation(i, otherfield->GetInterpolation(i));
    }

  this->GetUnit()->SetNumberOfValues(
      otherfield->GetUnit()->GetNumberOfValues());
  for(int i=0; i<this->GetUnit()->GetNumberOfValues(); i++)
    {
    this->GetUnit()->SetValue(i, otherfield->GetUnit()->GetValue(i));
    }

  int nstep = otherfield->GetNumberOfFieldStep();
  map<vtkMedFieldStep*, vtkMedFieldStep*> stepmap;
  for(int stepid=0; stepid<nstep; stepid++)
    {
    vtkMedFieldStep* otherstep = otherfield->GetFieldStep(stepid);
    vtkMedFieldStep* step = vtkMedFieldStep::New();
    step->SetComputeStep(otherstep->GetComputeStep());
    step->SetMedIterator(otherstep->GetMedIterator());
    this->AddFieldStep(step);
    step->Delete();

    stepmap[otherstep] = step;

    vtkMedFieldStep* previousstep = NULL;
    if(stepmap.find(otherstep->GetPreviousStep()) != stepmap.end())
      {
      previousstep = stepmap[otherstep->GetPreviousStep()];
      }
    step->SetPreviousStep(previousstep);
    step->SetParentField(this);
    step->SetMeshComputeStep(otherstep->GetMeshComputeStep());

    for(int eid=0; eid<otherstep->GetNumberOfFieldOverEntity(); eid++)
      {
      vtkMedFieldOverEntity* fieldOverEntity = otherstep->GetFieldOverEntity(eid);

      if(type == vtkMedField::PointField)
        {
        if(fieldOverEntity->GetEntity().EntityType != MED_NODE)
          {
          continue;
          }
        step->AppendFieldOverEntity(fieldOverEntity);
        otherstep->RemoveFieldOverEntity(fieldOverEntity);
        fieldOverEntity->SetParentStep(step);
        }
      else if(type == vtkMedField::ElnoField)
        {
        if(fieldOverEntity->GetEntity().EntityType != MED_NODE_ELEMENT)
          {
          continue;
          }

        step->AppendFieldOverEntity(fieldOverEntity);
        otherstep->RemoveFieldOverEntity(fieldOverEntity);
        eid--;
        fieldOverEntity->SetParentStep(step);
        }
      else
        {
        if(fieldOverEntity->GetEntity().EntityType == MED_NODE)
          {
          continue;
          }
        vtkMedFieldOverEntity* newfoe = vtkMedFieldOverEntity::New();
        newfoe->SetEntity(fieldOverEntity->GetEntity());
        newfoe->SetHasProfile(fieldOverEntity->GetHasProfile());
        newfoe->SetParentStep(step);
        step->AppendFieldOverEntity(newfoe);
        newfoe->Delete();
        for(int pid=0; pid<fieldOverEntity->GetNumberOfFieldOnProfile(); pid++)
          {
          vtkMedFieldOnProfile* fop = fieldOverEntity->GetFieldOnProfile(pid);
          const char* locname = fop->GetLocalizationName();
          if((type == vtkMedField::QuadratureField
             && strcmp(locname, MED_NO_LOCALIZATION) != 0) ||
             (type == vtkMedField::CellField
             && strcmp(locname, MED_NO_LOCALIZATION) == 0 ))
            {
            newfoe->AppendFieldOnProfile(fop);
            fieldOverEntity->RemoveFieldOnProfile(fop);
            pid--;
            fop->SetParentFieldOverEntity(newfoe);
            }
          }
        if(fieldOverEntity->GetNumberOfFieldOnProfile() == 0)
          {
          otherstep->RemoveFieldOverEntity(fieldOverEntity);
          eid--;
          }
        }
      }
    }

  this->ComputeFieldType();
  otherfield->ComputeFieldType();
}

void vtkMedField::SetNumberOfComponent(int ncomp)
{
  if (this->NumberOfComponent == ncomp)
    return;

  this->NumberOfComponent = ncomp;
  this->GetUnit()->SetNumberOfValues(this->NumberOfComponent);
  this->GetComponentName()->SetNumberOfValues(this->NumberOfComponent);

  this->Modified();
}

void  vtkMedField::AddFieldStep(vtkMedFieldStep* step)
{
  this->FieldStep->AddObject(step->GetComputeStep(), step);
}

void  vtkMedField::ClearFieldStep()
{
  this->FieldStep->clear();
}

vtkMedFieldStep* vtkMedField::GetFieldStep(const vtkMedComputeStep& cs)
{
  return this->FieldStep->GetObject(cs);
}

vtkMedFieldStep* vtkMedField::FindFieldStep(const vtkMedComputeStep& cs,
                                            int strategy)
{
  return this->FieldStep->FindObject(cs, strategy);
}

med_int vtkMedField::GetNumberOfFieldStep()
{
  return this->FieldStep->GetNumberOfObject();
}

vtkMedFieldStep* vtkMedField::GetFieldStep(med_int id)
{
  return this->FieldStep->GetObject(id);
}

void  vtkMedField::GatherFieldTimes(std::set<med_float>& times)
{
  this->FieldStep->GatherTimes(times);
}

void  vtkMedField::GatherFieldIterations(med_float time,
                                         std::set<med_int>& iterations)
{
  this->FieldStep->GatherIterations(time, iterations);
}

void vtkMedField::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  PRINT_IVAR(os, indent, MedIterator);
  PRINT_IVAR(os, indent, NumberOfComponent);
  PRINT_IVAR(os, indent, FieldType);
  PRINT_IVAR(os, indent, DataType);
  PRINT_IVAR(os, indent, Local);
}
