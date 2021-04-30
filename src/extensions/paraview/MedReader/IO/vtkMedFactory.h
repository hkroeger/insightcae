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

#ifndef __vtkMedFactory_h_
#define __vtkMedFactory_h_

#include "vtkObject.h"
class vtkMedDriver;

class VTK_EXPORT vtkMedFactory: public vtkObject
{
public:
  static vtkMedFactory* New();
  vtkTypeMacro(vtkMedFactory, vtkObject)
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMedDriver* NewMedDriver(int major, int minor, int release);

protected:
  vtkMedFactory();
  virtual ~vtkMedFactory();

private:
  vtkMedFactory(const vtkMedFactory&); // Not implemented.
  void operator=(const vtkMedFactory&); // Not implemented.
};

#endif //__vtkMedFactory_h_
