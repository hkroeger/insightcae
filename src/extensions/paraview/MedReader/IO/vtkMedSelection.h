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

#ifndef __vtkMedSelection_h_
#define __vtkMedSelection_h_

#include "vtkObject.h"

class vtkMedSelectionInternals;

class VTK_EXPORT vtkMedSelection : public vtkObject
{
public :
  static vtkMedSelection* New();
  vtkTypeMacro(vtkMedSelection, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void  Initialize();

  virtual void  AddKey(const char*);

  virtual void  SetKeyStatus(const char* name, int status);

  virtual int GetKeyStatus(const char* name);

  virtual const char* GetKey(int index);

  virtual int GetNumberOfKey();

  virtual int KeyExists(const char* name);

  virtual void  RemoveKeyByIndex(int index);

protected:
  vtkMedSelection();
  virtual ~vtkMedSelection();

 vtkMedSelectionInternals* Internals;

private:
  vtkMedSelection(const vtkMedSelection&); // Not implemented.
  void operator=(const vtkMedSelection&); // Not implemented.

};

#endif //__vtkMedSelection_h_
