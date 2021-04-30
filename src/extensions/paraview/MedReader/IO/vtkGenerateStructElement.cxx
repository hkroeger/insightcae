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

#include "vtkGenerateStructElement.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellData.h"
#include "vtkIdTypeArray.h"

#include "vtkMedUtilities.h"
#include "vtkMedStructElement.h"
#include "vtkMedConstantAttribute.h"
#include "vtkMedVariableAttribute.h"

class vtkGenerateStructElementCache
{
public :
  vtkGenerateStructElementCache(vtkMedStructElement* strelem, vtkUnstructuredGrid* ug)
    {
    for(int attid = 0; attid < strelem->GetNumberOfConstantAttribute(); attid++)
      {
      vtkMedConstantAttribute* att = strelem->GetConstantAttribute(attid);
      this->cstAttribute[att->GetName()] = att;
      }

    for(int attid = 0; attid < strelem->GetNumberOfVariableAttribute(); attid++)
      {
      vtkMedVariableAttribute* att = strelem->GetVariableAttribute(attid);
      vtkDataArray* array = ug->GetFieldData()->GetArray(att->GetName());
      if(array != NULL)
        this->varAttribute[att->GetName()] = array;
      }
    }

  bool HasParameter(const char* name)
    {
    return this->cstAttribute.find(name) != this->cstAttribute.end()
        || this->varAttribute.find(name) != this->varAttribute.end();
    }

  double GetParameter1(const char* name, vtkIdType id)
    {
    if(this->cstAttribute.find(name) != this->cstAttribute.end())
      {
      vtkMedConstantAttribute* att = this->cstAttribute[name];
      return att->GetValues()->GetVariantValue(0).ToDouble();
      }
    if(this->varAttribute.find(name) != this->varAttribute.end())
      {
      vtkDataArray* array = this->varAttribute[name];
      return array->GetTuple1(id);
      }
    return 0.0;
    }

protected :
  std::map<std::string, vtkMedConstantAttribute*> cstAttribute;
  std::map<std::string, vtkDataArray*> varAttribute;
};

vtkStandardNewMacro(vtkGenerateStructElement);

vtkGenerateStructElement::vtkGenerateStructElement()
{

}

vtkGenerateStructElement::~vtkGenerateStructElement()
{

}

int vtkGenerateStructElement::RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector)
{
  vtkInformation* outInfo=outputVector->GetInformationObject(0);

  vtkInformation* inputInfo=inputVector[0]->GetInformationObject(0);

  vtkUnstructuredGrid* inug = vtkUnstructuredGrid::SafeDownCast(
      inputInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkUnstructuredGrid* outug = vtkUnstructuredGrid::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));
  outug->Initialize();

  vtkMedStructElement* strelem = vtkMedStructElement::SafeDownCast(
      inug->GetInformation()->Get(vtkMedUtilities::STRUCT_ELEMENT()));

  if(strelem == NULL)
    {
    vtkDebugMacro("vtkGenerateStructElement needs a vtkMedStructElement information");
    return 1;
    }

  vtkIdTypeArray* strelemindex = vtkIdTypeArray::SafeDownCast(
      inug->GetCellData()->GetArray("STRUCT_ELEMENT_INDEX"));
  if(strelemindex == NULL)
    {
    vtkDebugMacro("vtkGenerateStructElement needs some information on the structural elements");
    return 1;
    }

  // loop over input cells.
  // struct elements support are a set cells of same type following each other.
  vtkIdType medid = -1;
  vtkGenerateStructElementCache cache(strelem, inug);

  std::string name = strelem->GetName();

  if(name == MED_BALL_NAME)
    {
    // sanity check : is the diameter defined?
    if(!cache.HasParameter(MED_BALL_DIAMETER))
      {
      vtkErrorMacro("MED_BALL elements need a diameter");
      return 1;
      }
    for(vtkIdType cellId = 0; cellId < inug->GetNumberOfCells(); cellId++)
      {
      vtkIdType ballMedId = strelemindex->GetValue(2*cellId);
      double balldiam = this->GetParameter1(MED_BALL_DIAMETER, ballMedId, cache);

      //TODO
      //this->GenerateBall(inug, cellId, balldiam, outug);
      }
    }
  else if(name == MED_PARTICLE_NAME)
    {
    bool hasLabel = cache.HasParameter(MED_PARTICLE_LABEL);
    for(vtkIdType cellId = 0; cellId < inug->GetNumberOfCells(); cellId++)
      {
      if(hasLabel)
        {
        vtkIdType particleMedId = strelemindex->GetValue(2*cellId);
        double particlelabel = this->GetParameter1(MED_PARTICLE_LABEL, particleMedId, cache);

        //TODO
        //  this->GenerateParticle(inug, cellId, particlelabel, outug);
        }
      else
        {
        //TODO
        //  this->GenerateParticle(inug, cellId, outug);
        }
      }
    }
  else if(name == MED_BEAM_NAME)
    {
    // sanity check : is the diameter defined?
    if(!cache.HasParameter(MED_BEAM_THICKNESS))
      {
      vtkErrorMacro("MED_BEAM elements need a thickness");
      return 1;
      }
    for(vtkIdType cellId = 0; cellId < inug->GetNumberOfCells(); cellId++)
      {
      vtkIdType cellmedid = strelemindex->GetValue(2*cellId);
      if(cellmedid != medid)
        {
        // this means that a new beam begins
        medid = cellmedid;
        double thickness = this->GetParameter1(MED_BEAM_THICKNESS, medid, cache);

        //TODO : generate a beam.
        // rem : a beam can span several segments.

        }
      }
    }
  else
    {
    vtkErrorMacro("structural elements of type " << name << " are not supported");
    }

  return 1;
}

double  vtkGenerateStructElement::GetParameter1(const char* name,
                              vtkIdType medid,
                              vtkGenerateStructElementCache& cache)
{
  return 0;
}

void  vtkGenerateStructElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
