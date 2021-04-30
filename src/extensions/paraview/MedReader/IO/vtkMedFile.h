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

#ifndef __vtkMedFile_h_
#define __vtkMedFile_h_

#include "vtkObject.h"
#include "vtkMedSetGet.h"

class vtkMedMesh;
class vtkMedField;
class vtkMedProfile;
class vtkMedLocalization;
class vtkMedLink;
class vtkMedDriver;
class vtkMedStructElement;
class vtkMedEntity;

class VTK_EXPORT vtkMedFile: public vtkObject
{
public:
  static vtkMedFile* New();
  vtkTypeMacro(vtkMedFile, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the file name to read from
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // This is the description of this file as stored in the med file.
  vtkGetObjectMacro(MedDriver, vtkMedDriver);

  // Description:
  // This method tries to create a new driver for this file.
  // It returns 1 on success, 0 on failure.
  virtual int CreateDriver();

  // Description:
  // read information from this file, and create the meta data structure
  virtual void  ReadInformation();

  // Description:
  // Container of the meshes.
  vtkGetObjectVectorMacro(Mesh, vtkMedMesh);
  vtkSetObjectVectorMacro(Mesh, vtkMedMesh);
  virtual vtkMedMesh* GetMesh(const char*);

  // Description:
  // Container of the fields.
  vtkGetObjectVectorMacro(Field, vtkMedField);
  vtkSetObjectVectorMacro(Field, vtkMedField);

  // Description:
  // Container of the fields.
  vtkGetObjectVectorMacro(Link, vtkMedLink);
  vtkSetObjectVectorMacro(Link, vtkMedLink);

  // Description:
  // Container of the profiles.
  vtkGetObjectVectorMacro(Profile, vtkMedProfile);
  vtkSetObjectVectorMacro(Profile, vtkMedProfile);
  virtual vtkMedProfile*  GetProfile(const char*);

  // Description:
  // Container of the quadrature definitions.
  vtkGetObjectVectorMacro(Localization, vtkMedLocalization);
  vtkSetObjectVectorMacro(Localization, vtkMedLocalization);
  virtual vtkMedLocalization* GetLocalization(const char*);

  // Description:
  // This is the description of this file as stored in the med file.
  vtkSetStringMacro(Comment);
  vtkGetStringMacro(Comment);

  // Description:
  // Those 3 numbers describe the version of med used to create this file.
  vtkSetMacro(VersionMajor, int);
  vtkGetMacro(VersionMajor, int);
  vtkSetMacro(VersionMinor, int);
  vtkGetMacro(VersionMinor, int);
  vtkSetMacro(VersionRelease, int);
  vtkGetMacro(VersionRelease, int);

  // Description:
  // Get the structural elements models
  vtkGetObjectVectorMacro(StructElement, vtkMedStructElement);
  vtkSetObjectVectorMacro(StructElement, vtkMedStructElement);
  vtkMedStructElement* GetStructElement(const vtkMedEntity&);

  // Description:
  // Get the support mesh informations
  vtkGetObjectVectorMacro(SupportMesh, vtkMedMesh);
  vtkSetObjectVectorMacro(SupportMesh, vtkMedMesh);

protected:
  vtkMedFile();
  virtual ~vtkMedFile();

  char * FileName;
  vtkMedDriver* MedDriver;
  virtual void  SetMedDriver(vtkMedDriver*);

  int VersionMajor;
  int VersionMinor;
  int VersionRelease;

  char* Comment;
  //BTX
  vtkObjectVector<vtkMedMesh>* Mesh;
  vtkObjectVector<vtkMedField>* Field;
  vtkObjectVector<vtkMedProfile>* Profile;
  vtkObjectVector<vtkMedLocalization>* Localization;
  vtkObjectVector<vtkMedLink>* Link;
  vtkObjectVector<vtkMedStructElement>* StructElement;
  vtkObjectVector<vtkMedMesh>* SupportMesh;
  //ETX

private:
  vtkMedFile(const vtkMedFile&); // Not implemented.
  void operator=(const vtkMedFile&); // Not implemented.
};

#endif //__vtkMedMetaData_h_
