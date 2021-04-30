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

#ifndef __vtkMedStructElement_h_
#define __vtkMedStructElement_h_

#include "vtkObject.h"
#include "vtkMed.h"
#include "vtkMedSetGet.h"

class vtkMedVariableAttribute;
class vtkMedConstantAttribute;
class vtkMedFile;
class vtkMedMesh;
class vtkMedEntityArray;

class VTK_EXPORT vtkMedStructElement : public vtkObject
{
public :
  static vtkMedStructElement* New();
  vtkTypeMacro(vtkMedStructElement, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The iterator to identify this struct element model in the med file
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // This is the name of this structural element model
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // The Geometry type this structural elements lies on.
  vtkSetMacro(GeometryType, med_geometry_type);
  vtkGetMacro(GeometryType, med_geometry_type);

  // Description:
  // The dimension of this structural elements model
  vtkSetMacro(ModelDimension, med_int);
  vtkGetMacro(ModelDimension, med_int);

  // Description:
  // The name of the support mesh used by this structural element model
  vtkSetStringMacro(SupportMeshName);
  vtkGetStringMacro(SupportMeshName);

  // Description:
  // This is the support mesh instance, corresponding to the above name.
  virtual void  SetSupportMesh(vtkMedMesh*);
  vtkGetObjectMacro(SupportMesh, vtkMedMesh);

  // Description:
  // The type of entity contained in the support mesh
  vtkSetMacro(SupportEntityType, med_entity_type);
  vtkGetMacro(SupportEntityType, med_entity_type);

  // Description:
  // The number of nodes of the support mesh
  vtkSetMacro(SupportNumberOfNode, med_int);
  vtkGetMacro(SupportNumberOfNode, med_int);

  // Description:
  // The number of cells in the support mesh.
  vtkSetMacro(SupportNumberOfCell, med_int);
  vtkGetMacro(SupportNumberOfCell, med_int);

  // Description:
  // The geometry type of the cells in the support mesh
  vtkSetMacro(SupportGeometryType, med_geometry_type);
  vtkGetMacro(SupportGeometryType, med_geometry_type);

  // Description:
  // This boolean is set to true if the attributes are given on a profile
  vtkSetMacro(AnyProfile, med_bool);
  vtkGetMacro(AnyProfile, med_bool);

  // Description:
  // Get the Variable Attributes
  vtkGetObjectVectorMacro(VariableAttribute, vtkMedVariableAttribute);
  vtkSetObjectVectorMacro(VariableAttribute, vtkMedVariableAttribute);

  // Description:
  // Get the Constant Attributes
  vtkGetObjectVectorMacro(ConstantAttribute, vtkMedConstantAttribute);
  vtkSetObjectVectorMacro(ConstantAttribute, vtkMedConstantAttribute);

  // Description:
  // The file this structural element model is stored in.
  virtual void SetParentFile(vtkMedFile*);
  vtkGetObjectMacro(ParentFile, vtkMedFile);

  // Description:
  // This method will use the driver to load the data for this compute step
  // for all variable attributes from disk.
  virtual void  LoadVariableAttributes(vtkMedEntityArray*);

  // Description:
  // returns the size of the connectivity for one structural element.
  // for MED_PARTICLE elements, it returns the number of nodes
  // if SupportEntityType == MED_CELL, it returns the number of cells in the
  // support mesh * the number of node for each cell.
  // else it returns the number of nodes in the support mesh.
  virtual int GetConnectivitySize();

protected:
  vtkMedStructElement();
  virtual ~vtkMedStructElement();

  med_int MedIterator;
  char *Name;
  med_geometry_type GeometryType;
  med_int ModelDimension;
  char *SupportMeshName;
  med_entity_type SupportEntityType;
  med_int SupportNumberOfNode;
  med_int SupportNumberOfCell;
  med_geometry_type SupportGeometryType;
  med_bool AnyProfile;

  vtkMedFile* ParentFile;
  vtkMedMesh* SupportMesh;

  vtkObjectVector<vtkMedVariableAttribute>* VariableAttribute;
  vtkObjectVector<vtkMedConstantAttribute>* ConstantAttribute;

private:
  vtkMedStructElement(const vtkMedStructElement&); // Not implemented.
  void operator=(const vtkMedStructElement&); // Not implemented.

};

#endif //__vtkMedStructElement_h_
