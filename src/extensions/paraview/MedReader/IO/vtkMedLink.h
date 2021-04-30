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

#ifndef __vtkMedLink_h_
#define __vtkMedLink_h_

#include "vtkObject.h"
#include "vtkMed.h"

#include <string>
#include <map>

class vtkMedString;

class VTK_EXPORT vtkMedLink: public vtkObject
{
public:
  static vtkMedLink* New();
  vtkTypeMacro(vtkMedLink, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // the index of this field in the med file.
  vtkSetMacro(MedIterator, med_int);
  vtkGetMacro(MedIterator, med_int);

  // Description:
  // Set the name of the mesh linked to.
  vtkSetStringMacro(MeshName);
  vtkGetStringMacro(MeshName);

  // Description:
  // Set the name of the file this link points to.
  vtkSetStringMacro(Link);
  vtkGetStringMacro(Link);

  // Description:
  // returns the full path to the linked file.
  // If the Link is already a full path, it is returned.
  // If the directory is a relative path, the returned path is the
  // concatenation of the directory where the
  // original file is in and the Link.
  const char* GetFullLink(const char* originalFileName);

  // Description:
  // this stores the iterator that should be used when unmounting this link
  void  SetMountedIterator(med_class, med_int);
  med_int GetMountedIterator(med_class);

protected:
  vtkMedLink();
  virtual ~vtkMedLink();

  med_int MedIterator;
  char* MeshName;
  char * Link;

  std::string FullLinkPath;

  // BTX
  std::map<med_class, int> Status;
  std::map<med_class, med_int> MountedIterator;
  //ETX

private:
  vtkMedLink(const vtkMedLink&); // Not implemented.
  void operator=(const vtkMedLink&); // Not implemented.

};

#endif //__vtkMedLink_h_
