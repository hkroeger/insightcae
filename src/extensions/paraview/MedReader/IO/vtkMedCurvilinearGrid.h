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

#ifndef __vtkMedCurvilinearGrid_h_
#define __vtkMedCurvilinearGrid_h_

#include "vtkMedGrid.h"
#include "vtkMed.h"

class vtkDataArray;
#include <vector>

class VTK_EXPORT vtkMedCurvilinearGrid : public vtkMedGrid
{
public :
  static vtkMedCurvilinearGrid* New();
  vtkTypeMacro(vtkMedCurvilinearGrid, vtkMedGrid)
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // overloaded to allocate memory for the grid size
  virtual void  SetDimension(int);
  virtual int GetDimension();

  // Description:
  // The number of nodes of this grid. This method do not
  // perform any allocation.
  vtkSetMacro(NumberOfPoints, med_int);
  vtkGetMacro(NumberOfPoints, med_int);

  // Description:
  // this array contains the coordinates of the points used by this grid.
  virtual void  SetCoordinates(vtkDataArray*);
  vtkGetObjectMacro(Coordinates, vtkDataArray);

  // Description:
  // Set/Get the size of each axis.
  // Note that the number of points must match the product of the
  // size of each axis.
  virtual void  SetAxisSize(int axis, med_int size);
  virtual med_int GetAxisSize(int axis);

  virtual void  LoadCoordinates();
  virtual int IsCoordinatesLoaded();

  virtual double* GetCoordTuple(med_int index);

  virtual vtkDataSet* CreateVTKDataSet(vtkMedFamilyOnEntityOnProfile*);

protected:
  vtkMedCurvilinearGrid();
  virtual ~vtkMedCurvilinearGrid();

  std::vector<med_int> AxisSize;
  med_int NumberOfPoints;
  vtkDataArray* Coordinates;

private:
  vtkMedCurvilinearGrid(const vtkMedCurvilinearGrid&); // Not implemented.
  void operator=(const vtkMedCurvilinearGrid&); // Not implemented.

};

#endif //__vtkMedCurvilinearGrid_h_
