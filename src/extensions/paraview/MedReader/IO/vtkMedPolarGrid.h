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

#ifndef __vtkMedPolarGrid_h_
#define __vtkMedPolarGrid_h_

#include "vtkMedRegularGrid.h"

class VTK_EXPORT vtkMedPolarGrid : public vtkMedRegularGrid
{
public :
  static vtkMedPolarGrid* New();
  vtkTypeMacro(vtkMedPolarGrid, vtkMedRegularGrid);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMedPolarGrid();
  virtual ~vtkMedPolarGrid();

private:
  vtkMedPolarGrid(const vtkMedPolarGrid&); // Not implemented.
  void operator=(const vtkMedPolarGrid&); // Not implemented.

};

#endif //__vtkMedPolarGrid_h_
